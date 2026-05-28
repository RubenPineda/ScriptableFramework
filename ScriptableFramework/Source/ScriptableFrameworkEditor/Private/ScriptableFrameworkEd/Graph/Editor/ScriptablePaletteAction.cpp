// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/ScriptablePaletteAction.h"
#include "ScriptableFrameworkEd/Graph/ScriptableGraphEditorHelpers.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableNodes/ScriptableNode_Task.h"
#include "ScriptableNodes/ScriptableNode_Exit.h"
#include "EdGraph/EdGraph.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "ScriptableTasks/ScriptableTask.h"
#include "ScriptableTasks/ScriptableTask_RunGraph.h"
#include "ScriptableTasks/ScriptableActionAsset.h"
#include "ScriptableNodes/ScriptableGraph.h"

namespace
{
	UEdGraphNode* WrapAssetInTaskNode(UEdGraph* ParentGraph, UClass* TaskClass, UObject* AssetToAssign, FName AssetFieldName, const FVector2f& Location, UEdGraphPin* FromPin)
	{
		UEdGraphNode* SpawnedNode = ScriptableGraphEditorHelpers::SpawnTaskNode(ParentGraph, TaskClass, Location, FromPin, /*bSelectNewNode*/ true);
		if (UScriptableEdGraphNode* SfEdNode = Cast<UScriptableEdGraphNode>(SpawnedNode))
		{
			if (UScriptableNode_Task* Wrapper = Cast<UScriptableNode_Task>(SfEdNode->GetRuntimeNode()))
			{
				if (UScriptableTask* Inner = Wrapper->Task)
				{
					Inner->Modify();

					// Set the named UObjectProperty by reflection, so RunAsset and RunGraph share one
					// path (field name is the only difference).
					if (FObjectProperty* ObjProp = FindFProperty<FObjectProperty>(Inner->GetClass(), AssetFieldName))
					{
						ObjProp->SetObjectPropertyValue_InContainer(Inner, AssetToAssign);
					}

					SfEdNode->ReconstructNode();
				}
			}
		}
		return SpawnedNode;
	}
}

UEdGraphNode* FScriptablePaletteAction::PerformAction(UEdGraph* ParentGraph, TArray<UEdGraphPin*>& FromPins, const FVector2f& Location, bool bSelectNewNode)
{
	if (!ParentGraph) return nullptr;

	UEdGraphPin* FromPin = (FromPins.Num() > 0) ? FromPins[0] : nullptr;

	if (AssetPayload.IsValid())
	{
		UObject* Asset = AssetPayload.GetAsset();
		if (Cast<UScriptableActionAsset>(Asset))
		{
			return WrapAssetInTaskNode(ParentGraph, UScriptableTask_RunAsset::StaticClass(), Asset, TEXT("Asset"), Location, FromPin);
		}
		if (Cast<UScriptableGraph>(Asset))
		{
			return WrapAssetInTaskNode(ParentGraph, UScriptableTask_RunGraph::StaticClass(), Asset, TEXT("GraphAsset"), Location, FromPin);
		}
		return nullptr;
	}

	const UClass* Class = Cast<UClass>(StructPayload);
	if (!Class) return nullptr;

	if (Class->IsChildOf(UScriptableTask::StaticClass()))
	{
		return ScriptableGraphEditorHelpers::SpawnTaskNode(ParentGraph, const_cast<UClass*>(Class), Location, FromPin, bSelectNewNode);
	}
	if (Class->IsChildOf(UScriptableNode::StaticClass()))
	{
		// Exit is unique per graph: refuse a second one (the validator also enforces this).
		if (Class->IsChildOf(UScriptableNode_Exit::StaticClass()))
		{
			for (const UEdGraphNode* EdNode : ParentGraph->Nodes)
			{
				const UScriptableEdGraphNode* SfEd = Cast<UScriptableEdGraphNode>(EdNode);
				if (SfEd && SfEd->GetRuntimeNode() && SfEd->GetRuntimeNode()->IsA<UScriptableNode_Exit>())
				{
					FNotificationInfo Info(NSLOCTEXT("ScriptablePaletteAction", "OneExit", "A graph can only have one Exit node."));
					Info.ExpireDuration = 3.0f;
					FSlateNotificationManager::Get().AddNotification(Info);
					return nullptr;
				}
			}
		}

		return ScriptableGraphEditorHelpers::SpawnNativeNode(ParentGraph, const_cast<UClass*>(Class), Location, FromPin, bSelectNewNode);
	}

	return nullptr;
}
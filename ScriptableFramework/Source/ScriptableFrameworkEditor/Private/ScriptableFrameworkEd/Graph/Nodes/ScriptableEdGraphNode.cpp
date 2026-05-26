// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableNodes/ScriptableGraph.h"
#include "ScriptableFrameworkEditorStyle.h"
#include "ScriptableFrameworkEditorHelpers.h"
#include "EdGraph/EdGraph.h"

const FName UScriptableEdGraphNode::ScriptableExecPinCategory = TEXT("ScriptableExec");

void UScriptableEdGraphNode::AllocateDefaultPins()
{
	if (!RuntimeNode) return;

	for (const FName& InputName : RuntimeNode->GetInputPins())
	{
		CreatePin(EGPD_Input, ScriptableExecPinCategory, InputName);
	}

	for (const FName& OutputName : RuntimeNode->GetOutputPins())
	{
		CreatePin(EGPD_Output, ScriptableExecPinCategory, OutputName);
	}
}

FText UScriptableEdGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (RuntimeNode)
	{
		return RuntimeNode->GetClass()->GetDisplayNameText();
	}
	return NSLOCTEXT("ScriptableEdGraphNode", "DefaultTitle", "Scriptable Node");
}

FLinearColor UScriptableEdGraphNode::GetNodeTitleColor() const
{
	if (RuntimeNode)
	{
		const FString Category = RuntimeNode->GetClass()->GetMetaData(ScriptableFrameworkEditor::MD_NodeCategory);
		if (!Category.IsEmpty())
		{
			if (Category.StartsWith(TEXT("Condition"), ESearchCase::IgnoreCase)) return FScriptableFrameworkEditorStyle::ScriptableConditionColor;
			if (Category.StartsWith(TEXT("System"), ESearchCase::IgnoreCase)) return FScriptableFrameworkEditorStyle::ScriptableSystemNodeColor;
		}
	}

	return FScriptableFrameworkEditorStyle::ScriptableTaskColor;
}

void UScriptableEdGraphNode::DestroyNode()
{
	// Capture the asset reference before Super::DestroyNode unhooks us from the graph.
	UEdGraph* OwningGraph = GetGraph();
	UScriptableGraph* GraphAsset = OwningGraph ? Cast<UScriptableGraph>(OwningGraph->GetOuter()) : nullptr;
	UScriptableNode* RuntimeNodeToRemove = RuntimeNode;

	Super::DestroyNode();

	// Entry node is auto-repaired by UScriptableGraph::EnsureEntryNode; never strip it from the asset.
	if (!GraphAsset || !RuntimeNodeToRemove) return;
	if (RuntimeNodeToRemove->GetBindingID() == GraphAsset->EntryNodeID) return;

	GraphAsset->Modify();
	GraphAsset->Nodes.Remove(RuntimeNodeToRemove);
}

void UScriptableEdGraphNode::ReconstructNode()
{
	TArray<UEdGraphPin*> OldPins = MoveTemp(Pins);
	Pins.Reset();

	AllocateDefaultPins();

	for (UEdGraphPin* OldPin : OldPins)
	{
		if (!OldPin) continue;

		if (UEdGraphPin* NewPin = FindPin(OldPin->PinName, OldPin->Direction))
		{
			for (UEdGraphPin* Linked : OldPin->LinkedTo)
			{
				if (Linked) NewPin->MakeLinkTo(Linked);
			}
		}

		// Sever the old pin from its peers manually to avoid triggering the Schema.
		// Calling BreakAllPinLinks here would corrupt Asset->Connections during reconstruction.
		for (UEdGraphPin* Linked : OldPin->LinkedTo)
		{
			if (Linked)
			{
				Linked->LinkedTo.Remove(OldPin);
			}
		}
		OldPin->LinkedTo.Empty();
	}

	for (UEdGraphPin* OldPin : OldPins)
	{
		if (OldPin) DestroyPin(OldPin);
	}

	if (UEdGraph* OwningGraph = GetGraph())
	{
		OwningGraph->NotifyGraphChanged();
	}
}
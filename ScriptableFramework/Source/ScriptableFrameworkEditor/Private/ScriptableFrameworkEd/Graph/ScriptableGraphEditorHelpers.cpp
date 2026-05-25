// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/ScriptableGraphEditorHelpers.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode_Entry.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode_Task.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphSchema.h"
#include "ScriptableNodes/ScriptableGraph.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableNodes/ScriptableNode_Entry.h"
#include "ScriptableNodes/ScriptableNode_Task.h"
#include "ScriptableTasks/ScriptableTask.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "ScriptableGraphEditorHelpers"

namespace ScriptableGraphEditorHelpers
{
	UEdGraphNode* SpawnTaskNode(UEdGraph* ParentGraph, TSubclassOf<UScriptableTask> TaskClass, const FVector2f& Location, UEdGraphPin* FromPin, bool bSelectNewNode)
	{
		if (!ParentGraph || !TaskClass) return nullptr;

		UScriptableGraph* GraphAsset = Cast<UScriptableGraph>(ParentGraph->GetOuter());
		if (!GraphAsset) return nullptr;

		const FScopedTransaction Transaction(LOCTEXT("AddTaskNode", "Add Task Node"));
		GraphAsset->Modify();
		ParentGraph->Modify();

		// Create the runtime wrapper owned by the asset, with the task instantiated inside it.
		UScriptableNode_Task* RuntimeNode = NewObject<UScriptableNode_Task>(GraphAsset, NAME_None, RF_Transactional);
		RuntimeNode->Task = NewObject<UScriptableTask>(RuntimeNode, TaskClass, NAME_None, RF_Transactional);
		GraphAsset->Nodes.Add(RuntimeNode);

		// Spawn the matching visual node.
		UScriptableEdGraphNode_Task* EdNode = NewObject<UScriptableEdGraphNode_Task>(ParentGraph, UScriptableEdGraphNode_Task::StaticClass(), NAME_None, RF_Transactional);
		EdNode->SetRuntimeNode(RuntimeNode);
		EdNode->CreateNewGuid();
		EdNode->NodePosX = Location.X;
		EdNode->NodePosY = Location.Y;
		EdNode->AllocateDefaultPins();
		ParentGraph->AddNode(EdNode, /*bUserAction*/ true, bSelectNewNode);

		AutoWireFromPin(FromPin, EdNode);
		return EdNode;
	}

	UEdGraphNode* SpawnNativeNode(UEdGraph* ParentGraph, TSubclassOf<UScriptableNode> NodeClass, const FVector2f& Location, UEdGraphPin* FromPin, bool bSelectNewNode)
	{
		if (!ParentGraph || !NodeClass) return nullptr;

		UScriptableGraph* GraphAsset = Cast<UScriptableGraph>(ParentGraph->GetOuter());
		if (!GraphAsset) return nullptr;

		const FScopedTransaction Transaction(LOCTEXT("AddNativeNode", "Add Node"));
		GraphAsset->Modify();
		ParentGraph->Modify();

		// Plain native node (Branch, Sequence, etc.). No wrapper, no inner task.
		UScriptableNode* RuntimeNode = NewObject<UScriptableNode>(GraphAsset, NodeClass, NAME_None, RF_Transactional);
		GraphAsset->Nodes.Add(RuntimeNode);

		// For now, native nodes share the base UScriptableEdGraphNode visual. Specialized visuals
		// can be introduced later by mapping runtime class -> ed-node class.
		UScriptableEdGraphNode* EdNode = NewObject<UScriptableEdGraphNode>(ParentGraph, UScriptableEdGraphNode::StaticClass(), NAME_None, RF_Transactional);
		EdNode->SetRuntimeNode(RuntimeNode);
		EdNode->CreateNewGuid();
		EdNode->NodePosX = Location.X;
		EdNode->NodePosY = Location.Y;
		EdNode->AllocateDefaultPins();
		ParentGraph->AddNode(EdNode, /*bUserAction*/ true, bSelectNewNode);

		AutoWireFromPin(FromPin, EdNode);
		return EdNode;
	}

	void AutoWireFromPin(UEdGraphPin* FromPin, UEdGraphNode* TargetNode)
	{
		if (!FromPin || !TargetNode) return;

		// Drag was from an output pin → look for the new node's first input pin.
		// Drag was from an input pin → look for the new node's first output pin.
		// Pin categories filtered to ScriptableExec so we don't try to bridge incompatible pin systems.
		const EEdGraphPinDirection TargetDir = (FromPin->Direction == EGPD_Output) ? EGPD_Input : EGPD_Output;

		UEdGraphPin* TargetPin = nullptr;
		for (UEdGraphPin* Pin : TargetNode->Pins)
		{
			if (Pin && Pin->Direction == TargetDir && Pin->PinType.PinCategory == UScriptableEdGraphNode::ScriptableExecPinCategory)
			{
				TargetPin = Pin;
				break;
			}
		}

		if (!TargetPin) return;

		const UEdGraphSchema* Schema = TargetNode->GetSchema();
		if (!Schema) return;

		Schema->TryCreateConnection(FromPin, TargetPin);
	}

	UScriptableEdGraphNode* SpawnEdNodeForRuntime(UEdGraph* ParentGraph, UScriptableNode* RuntimeNode, const FVector2f& Location)
	{
		if (!ParentGraph || !RuntimeNode) return nullptr;

		UScriptableEdGraphNode* NewEdNode = nullptr;
		if (RuntimeNode->IsA<UScriptableNode_Entry>())
		{
			NewEdNode = NewObject<UScriptableEdGraphNode_Entry>(ParentGraph, UScriptableEdGraphNode_Entry::StaticClass(), NAME_None, RF_Transactional);
		}
		else if (RuntimeNode->IsA<UScriptableNode_Task>())
		{
			NewEdNode = NewObject<UScriptableEdGraphNode_Task>(ParentGraph, UScriptableEdGraphNode_Task::StaticClass(), NAME_None, RF_Transactional);
		}
		else
		{
			NewEdNode = NewObject<UScriptableEdGraphNode>(ParentGraph, UScriptableEdGraphNode::StaticClass(), NAME_None, RF_Transactional);
		}

		if (NewEdNode)
		{
			NewEdNode->SetRuntimeNode(RuntimeNode);
			NewEdNode->CreateNewGuid();
			NewEdNode->NodePosX = Location.X;
			NewEdNode->NodePosY = Location.Y;
			NewEdNode->SnapToGrid(16);
			NewEdNode->AllocateDefaultPins();
			ParentGraph->AddNode(NewEdNode, /*bUserAction*/ true, /*bSelectNewNode*/ false);
		}

		return NewEdNode;
	}
}

#undef LOCTEXT_NAMESPACE
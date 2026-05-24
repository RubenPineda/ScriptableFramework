// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/ScriptableGraphEditorHelpers.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode_Entry.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode_Task.h"
#include "ScriptableNodes/ScriptableGraph.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableNodes/ScriptableNode_Entry.h"
#include "ScriptableNodes/ScriptableNode_Task.h"
#include "ScriptableTasks/ScriptableTask.h"
#include "EdGraph/EdGraph.h"
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

		// TODO: when connection support lands, auto-wire FromPin to EdNode's matching input.
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

		return EdNode;
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
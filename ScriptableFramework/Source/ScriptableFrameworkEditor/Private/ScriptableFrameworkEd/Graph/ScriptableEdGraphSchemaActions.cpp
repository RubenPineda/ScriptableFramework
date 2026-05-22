// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphSchemaActions.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode_Task.h"
#include "ScriptableNodes/ScriptableGraph.h"
#include "ScriptableNodes/ScriptableNode_Task.h"
#include "ScriptableTasks/ScriptableTask.h"
#include "EdGraph/EdGraph.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "ScriptableEdGraphSchemaActions"

UEdGraphNode* FScriptableEdGraphSchemaAction_NewTaskNode::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2f& Location, bool bSelectNewNode)
{
	if (!ParentGraph || !TaskClass) return nullptr;

	UScriptableGraph* GraphAsset = Cast<UScriptableGraph>(ParentGraph->GetOuter());
	if (!GraphAsset) return nullptr;

	const FScopedTransaction Transaction(LOCTEXT("AddTaskNode", "Add Task Node"));
	GraphAsset->Modify();
	ParentGraph->Modify();

	// Create the wrapper runtime node owned by the asset, with the task instantiated inside it.
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

	return EdNode;
}

#undef LOCTEXT_NAMESPACE
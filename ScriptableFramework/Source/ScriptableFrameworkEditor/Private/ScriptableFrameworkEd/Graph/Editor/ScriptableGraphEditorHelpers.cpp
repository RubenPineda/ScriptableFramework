// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/ScriptableGraphEditorHelpers.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNodeRegistry.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphSchema.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode_Task.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode_Native.h"
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

		UClass* EdNodeClass = FScriptableEdGraphNodeRegistry::FindEdNodeClassFor(RuntimeNode);
		if (!EdNodeClass) EdNodeClass = UScriptableEdGraphNode_Native::StaticClass();

		UScriptableEdGraphNode* EdNode = NewObject<UScriptableEdGraphNode>(ParentGraph, EdNodeClass, NAME_None, RF_Transactional);
		EdNode->SetRuntimeNode(RuntimeNode);
		EdNode->CreateNewGuid();
		EdNode->NodePosX = Location.X;
		EdNode->NodePosY = Location.Y;
		EdNode->AllocateDefaultPins();
		ParentGraph->AddNode(EdNode, /*bUserAction*/ true, bSelectNewNode);

		AutoWireFromPin(FromPin, EdNode);
		return EdNode;
	}

	namespace
	{
		/** First ScriptableExec pin on Node in the given direction, or null. */
		UEdGraphPin* FindFirstScriptableExecPin(UEdGraphNode* Node, EEdGraphPinDirection Direction)
		{
			if (!Node) return nullptr;
			for (UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin && Pin->Direction == Direction && Pin->PinType.PinCategory == UScriptableEdGraphNode::ScriptableExecPinCategory)
				{
					return Pin;
				}
			}
			return nullptr;
		}
	}

	void AutoWireFromPin(UEdGraphPin* FromPin, UEdGraphNode* TargetNode)
	{
		if (!FromPin || !TargetNode) return;

		const UEdGraphSchema* Schema = TargetNode->GetSchema();
		if (!Schema) return;

		// Incoming-side pin on the new node: opposite direction to FromPin (output drag → first input,
		// input drag → first output). This is the side that absorbs the dragged wire.
		const EEdGraphPinDirection IncomingDir = (FromPin->Direction == EGPD_Output) ? EGPD_Input : EGPD_Output;
		UEdGraphPin* IncomingPin = FindFirstScriptableExecPin(TargetNode, IncomingDir);
		if (!IncomingPin) return;

		// Splice the new node into the dragged wire IF (a) FromPin was already connected to something
		// and (b) the new node has at least one matching-direction output pin to forward the signal to.
		// Without an output pin (e.g. Exit) we can't be the middle of a chain, so we just fan in
		// without breaking the original wire — same as the legacy single-connection behaviour.
		const TArray<UEdGraphPin*> ExistingPeers = FromPin->LinkedTo;
		UEdGraphPin* OutgoingPin = FindFirstScriptableExecPin(TargetNode, FromPin->Direction);

		if (OutgoingPin && !ExistingPeers.IsEmpty())
		{
			for (UEdGraphPin* Peer : ExistingPeers)
			{
				if (!Peer) continue;
				// Break the old direct wire and re-route it through TargetNode's first output.
				// Picking the first output matches the rule the user wanted for ambiguity.
				FromPin->BreakLinkTo(Peer);
				Schema->TryCreateConnection(OutgoingPin, Peer);
			}
		}

		Schema->TryCreateConnection(FromPin, IncomingPin);
	}

	UScriptableEdGraphNode* SpawnEdNodeForRuntime(UEdGraph* ParentGraph, UScriptableNode* RuntimeNode, const FVector2f& Location)
	{
		if (!ParentGraph || !RuntimeNode) return nullptr;

		UClass* EdNodeClass = FScriptableEdGraphNodeRegistry::FindEdNodeClassFor(RuntimeNode);
		if (!EdNodeClass) EdNodeClass = UScriptableEdGraphNode_Native::StaticClass();

		UScriptableEdGraphNode* NewEdNode = NewObject<UScriptableEdGraphNode>(ParentGraph, EdNodeClass, NAME_None, RF_Transactional);

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
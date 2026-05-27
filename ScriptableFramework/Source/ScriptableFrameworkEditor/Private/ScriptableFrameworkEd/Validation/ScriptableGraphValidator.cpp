// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Validation/ScriptableGraphValidator.h"
#include "ScriptableNodes/ScriptableGraph.h"
#include "ScriptableNodes/ScriptableGraphConnection.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableNodes/ScriptableNode_ReceiveEvent.h"
#include "ScriptableNodes/ScriptableNode_Task.h"
#include "ScriptableTasks/ScriptableTask.h"
#include "ScriptableTasks/ScriptableTask_RunGraph.h"

#define LOCTEXT_NAMESPACE "ScriptableGraphValidator"

namespace
{
	const FName GValidatorId("ScriptableGraph");

	/** Readable label for messages: the inner task's class name for Task wrappers, else the node's class display name. */
	FString GetNodeLabel(const UScriptableNode* Node)
	{
		if (!Node) return TEXT("<null>");
		if (const UScriptableNode_Task* TaskNode = Cast<UScriptableNode_Task>(Node))
		{
			if (TaskNode->Task) return TaskNode->Task->GetClass()->GetDisplayNameText().ToString();
		}
		return Node->GetClass()->GetDisplayNameText().ToString();
	}

	/**
	 * Recursive DFS that flags cycles. Nodes on the current path live in InStack; fully explored ones
	 * in Visited (shared across seeds so each node is walked once). A back-edge to an InStack node closes
	 * a cycle, reported once per closing node with the path slice that forms the loop.
	 */
	void DetectCycleDFS(const FGuid& NodeId, const TMultiMap<FGuid, FGuid>& Adjacency, const TMap<FGuid, const UScriptableNode*>& NodesByGuid,
		TSet<FGuid>& Visited, TSet<FGuid>& InStack, TArray<FGuid>& PathStack, TSet<FGuid>& ReportedCycles, TArray<FKzValidationIssue>& OutIssues)
	{
		Visited.Add(NodeId);
		InStack.Add(NodeId);
		PathStack.Add(NodeId);

		TArray<FGuid> Targets;
		Adjacency.MultiFind(NodeId, Targets);
		for (const FGuid& Target : Targets)
		{
			if (InStack.Contains(Target))
			{
				if (ReportedCycles.Contains(Target)) continue;
				ReportedCycles.Add(Target);

				// Build "Closing -> ... -> Current -> Closing" from the path slice starting at the closing node.
				FString PathStr;
				const int32 StartIdx = PathStack.Find(Target);
				for (int32 i = StartIdx; i < PathStack.Num(); ++i)
				{
					PathStr += GetNodeLabel(NodesByGuid.FindRef(PathStack[i]));
					PathStr += TEXT(" \u2192 ");
				}
				PathStr += GetNodeLabel(NodesByGuid.FindRef(Target));

				OutIssues.Add(FKzValidationIssue::WithContextId(EKzValidationSeverity::Error,
					FText::Format(LOCTEXT("Cycle", "Cycle detected at node '{0}' (path: {1})."),
						FText::FromString(GetNodeLabel(NodesByGuid.FindRef(Target))), FText::FromString(PathStr)),
					GValidatorId, Target));
			}
			else if (!Visited.Contains(Target))
			{
				DetectCycleDFS(Target, Adjacency, NodesByGuid, Visited, InStack, PathStack, ReportedCycles, OutIssues);
			}
		}

		InStack.Remove(NodeId);
		PathStack.Pop();
	}

	/**
	 * DFS over assets following RunGraph references. Returns true if Current eventually reaches Origin,
	 * collecting the asset chain (excluding Origin) in OutPath. Null/unloaded GraphAssets are skipped.
	 */
	bool RunGraphReachesOrigin(const UScriptableGraph* Current, const UScriptableGraph* Origin, TSet<const UScriptableGraph*>& Visited, TArray<const UScriptableGraph*>& OutPath)
	{
		if (!Current) return false;
		if (Current == Origin) return true;
		if (Visited.Contains(Current)) return false;

		Visited.Add(Current);
		OutPath.Add(Current);

		for (const TObjectPtr<UScriptableNode>& Node : Current->Nodes)
		{
			const UScriptableNode_Task* TaskNode = Cast<UScriptableNode_Task>(Node);
			if (!TaskNode) continue;
			const UScriptableTask_RunGraph* RunGraph = Cast<UScriptableTask_RunGraph>(TaskNode->Task);
			if (!RunGraph || !RunGraph->GraphAsset) continue;

			if (RunGraphReachesOrigin(RunGraph->GraphAsset, Origin, Visited, OutPath))
			{
				return true;
			}
		}

		OutPath.Pop();
		return false;
	}
}

FName UScriptableGraphValidator::GetValidatorId_Implementation() const
{
	return GValidatorId;
}

bool UScriptableGraphValidator::CanValidate_Implementation(const UObject* Asset) const
{
	return Asset && Asset->IsA<UScriptableGraph>();
}

void UScriptableGraphValidator::Validate_Implementation(const UObject* Asset, TArray<FKzValidationIssue>& OutIssues) const
{
	const UScriptableGraph* Graph = Cast<UScriptableGraph>(Asset);
	if (!Graph) return;

	// --- Node-by-GUID lookup. Duplicate IDs corrupt connection routing (error); first occurrence wins. ---
	TMap<FGuid, const UScriptableNode*> NodesByGuid;
	NodesByGuid.Reserve(Graph->Nodes.Num());
	for (const TObjectPtr<UScriptableNode>& Node : Graph->Nodes)
	{
		if (!Node) continue;
		const FGuid Id = Node->GetBindingID();
		if (!Id.IsValid()) continue;

		if (const UScriptableNode** Existing = NodesByGuid.Find(Id))
		{
			OutIssues.Add(FKzValidationIssue::WithContextId(EKzValidationSeverity::Error,
				FText::Format(LOCTEXT("DuplicateId", "Node '{0}' shares its ID with '{1}' (duplicate node ID corrupts connection routing)."),
					FText::FromString(GetNodeLabel(Node)), FText::FromString(GetNodeLabel(*Existing))),
				GValidatorId, Id));
			continue;
		}
		NodesByGuid.Add(Id, Node);
	}

	// --- Entry points: EntryNodeID + every ReceiveEvent. Also flag ReceiveEvents with no EventName. ---
	TArray<FGuid> Seeds;
	if (Graph->EntryNodeID.IsValid() && NodesByGuid.Contains(Graph->EntryNodeID))
	{
		Seeds.AddUnique(Graph->EntryNodeID);
	}
	for (const TObjectPtr<UScriptableNode>& Node : Graph->Nodes)
	{
		const UScriptableNode_ReceiveEvent* Receive = Cast<UScriptableNode_ReceiveEvent>(Node);
		if (!Receive) continue;

		const FGuid Id = Receive->GetBindingID();
		if (Id.IsValid()) Seeds.AddUnique(Id);

		if (Receive->EventName.IsNone())
		{
			OutIssues.Add(FKzValidationIssue::WithContextId(EKzValidationSeverity::Warning,
				LOCTEXT("EmptyEventName", "ReceiveEvent node has empty EventName and will never fire."),
				GValidatorId, Id));
		}
	}

	// --- Connection validation + forward adjacency (From -> To) used by reachability and cycle checks. ---
	TMultiMap<FGuid, FGuid> Adjacency;
	for (const FScriptableGraphConnection& Conn : Graph->Connections)
	{
		const UScriptableNode* FromNode = NodesByGuid.FindRef(Conn.From.NodeID);
		const UScriptableNode* ToNode = NodesByGuid.FindRef(Conn.To.NodeID);

		// Missing node endpoint: one issue per connection (From wins when both are missing).
		if (!FromNode || !ToNode)
		{
			const FGuid MissingId = !FromNode ? Conn.From.NodeID : Conn.To.NodeID;
			OutIssues.Add(FKzValidationIssue::WithContextId(EKzValidationSeverity::Error,
				FText::Format(LOCTEXT("MissingNode", "Connection references missing node {0}."),
					FText::FromString(MissingId.ToString(EGuidFormats::DigitsWithHyphens))),
				GValidatorId, MissingId));
			continue;
		}

		// Both nodes exist: they form a forward edge regardless of pin correctness.
		Adjacency.Add(Conn.From.NodeID, Conn.To.NodeID);

		const TArray<FName> FromOutputs = FromNode->GetOutputPins();
		const TArray<FName> FromInputs = FromNode->GetInputPins();
		const TArray<FName> ToInputs = ToNode->GetInputPins();
		const TArray<FName> ToOutputs = ToNode->GetOutputPins();

		const bool bFromIsOutput = FromOutputs.Contains(Conn.From.PinName);
		const bool bToIsInput = ToInputs.Contains(Conn.To.PinName);

		// Inverted wire: From lands on an input pin, or To lands on an output pin. One issue, keyed to From.
		if ((!bFromIsOutput && FromInputs.Contains(Conn.From.PinName)) || (!bToIsInput && ToOutputs.Contains(Conn.To.PinName)))
		{
			OutIssues.Add(FKzValidationIssue::WithContextId(EKzValidationSeverity::Error,
				FText::Format(LOCTEXT("Inverted", "Connection has inverted direction on node '{0}'."), FText::FromString(GetNodeLabel(FromNode))),
				GValidatorId, Conn.From.NodeID));
			continue;
		}

		// Otherwise, flag any pin that simply does not exist on its node.
		if (!bFromIsOutput)
		{
			OutIssues.Add(FKzValidationIssue::WithContextId(EKzValidationSeverity::Error,
				FText::Format(LOCTEXT("MissingPinFrom", "Connection references missing pin '{0}' on node '{1}'."),
					FText::FromName(Conn.From.PinName), FText::FromString(GetNodeLabel(FromNode))),
				GValidatorId, Conn.From.NodeID));
		}
		if (!bToIsInput)
		{
			OutIssues.Add(FKzValidationIssue::WithContextId(EKzValidationSeverity::Error,
				FText::Format(LOCTEXT("MissingPinTo", "Connection references missing pin '{0}' on node '{1}'."),
					FText::FromName(Conn.To.PinName), FText::FromString(GetNodeLabel(ToNode))),
				GValidatorId, Conn.To.NodeID));
		}
	}

	// --- Reachability: BFS from the seed set. Any node never reached is an orphan (warning). ---
	TSet<FGuid> Reachable(Seeds);
	TArray<FGuid> Queue = Seeds;
	for (int32 Head = 0; Head < Queue.Num(); ++Head)
	{
		TArray<FGuid> Targets;
		Adjacency.MultiFind(Queue[Head], Targets);
		for (const FGuid& Target : Targets)
		{
			bool bAlready = false;
			Reachable.Add(Target, &bAlready);
			if (!bAlready) Queue.Add(Target);
		}
	}

	for (const TObjectPtr<UScriptableNode>& Node : Graph->Nodes)
	{
		if (!Node) continue;
		const FGuid Id = Node->GetBindingID();
		if (!Id.IsValid() || Reachable.Contains(Id)) continue;

		OutIssues.Add(FKzValidationIssue::WithContextId(EKzValidationSeverity::Warning,
			FText::Format(LOCTEXT("Orphan", "Node '{0}' is not reachable from Entry or any ReceiveEvent."), FText::FromString(GetNodeLabel(Node))),
			GValidatorId, Id));
	}

	// --- Cycle detection: DFS from every entry point. ---
	{
		TSet<FGuid> Visited;
		TSet<FGuid> InStack;
		TArray<FGuid> PathStack;
		TSet<FGuid> ReportedCycles;
		const int32 SeedCount = Seeds.Num();
		for (int32 i = 0; i < SeedCount; ++i)
		{
			if (!Visited.Contains(Seeds[i]))
			{
				DetectCycleDFS(Seeds[i], Adjacency, NodesByGuid, Visited, InStack, PathStack, ReportedCycles, OutIssues);
			}
		}
	}

	// --- RunGraph recursion: direct self-reference (error) and indirect cross-asset cycle (warning). ---
	for (const TObjectPtr<UScriptableNode>& Node : Graph->Nodes)
	{
		const UScriptableNode_Task* TaskNode = Cast<UScriptableNode_Task>(Node);
		if (!TaskNode) continue;
		const UScriptableTask_RunGraph* RunGraph = Cast<UScriptableTask_RunGraph>(TaskNode->Task);
		if (!RunGraph) continue;

		const FGuid Id = Node->GetBindingID();

		if (RunGraph->GraphAsset == Graph)
		{
			OutIssues.Add(FKzValidationIssue::WithContextId(EKzValidationSeverity::Error,
				FText::Format(LOCTEXT("RunGraphSelf", "RunGraph node '{0}' references the asset itself (infinite recursion)."), FText::FromString(GetNodeLabel(Node))),
				GValidatorId, Id));
			continue;
		}

		if (!RunGraph->GraphAsset) continue;

		// Does following this reference (between assets) lead back to us?
		TSet<const UScriptableGraph*> Visited;
		TArray<const UScriptableGraph*> PathAssets;
		if (RunGraphReachesOrigin(RunGraph->GraphAsset, Graph, Visited, PathAssets))
		{
			FString PathStr = Graph->GetName();
			for (const UScriptableGraph* PathAsset : PathAssets)
			{
				PathStr += TEXT(" \u2192 ");
				PathStr += PathAsset ? PathAsset->GetName() : TEXT("?");
			}
			PathStr += TEXT(" \u2192 ");
			PathStr += Graph->GetName();

			OutIssues.Add(FKzValidationIssue::WithContextId(EKzValidationSeverity::Warning,
				FText::Format(LOCTEXT("RunGraphIndirect", "RunGraph node '{0}' may form indirect cycle: {1}."), FText::FromString(GetNodeLabel(Node)), FText::FromString(PathStr)),
				GValidatorId, Id));
		}
	}
}

#undef LOCTEXT_NAMESPACE

// Copyright 2026 kirzo

#include "ScriptableNodes/ScriptableGraphInstance.h"
#include "ScriptableNodes/ScriptableGraph.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableNodes/ScriptableNode_Entry.h"
#include "ScriptableNodes/ScriptableNode_Exit.h"
#include "ScriptableNodes/ScriptableNode_ReceiveEvent.h"
#include "ScriptableNodes/ScriptableGraphSubsystem.h"
#include "ScriptableContext.h"
#include "ScriptableObject.h"

void UScriptableGraphInstance::Launch(UScriptableGraph* InAsset, UObject* InOwner, const FScriptableContext& InContext)
{
	if (!InAsset || !InOwner) return;

#if WITH_EDITOR
	/** Editor-only gate: refuse to run a graph whose last compile failed. The editor module surfaces a dialog after PIE. */
	if (InAsset->bLastCompileFailed)
	{
		UE_LOG(LogScriptableObject, Error, TEXT("ScriptableGraph '%s' refused to launch: last compile failed. Open the asset and hit Compile to see the issues."), *InAsset->GetName());
		UScriptableGraph::OnLaunchBlockedByCompile.Broadcast(InAsset);
		return;
	}
#endif

	Asset = InAsset;
	Owner = InOwner;

	// Register with the world subsystem: it owns the live-runner set (keeping us alive while we run)
	// and cancels us on world teardown. No subsystem means the owner has no world (CDO/transient) -> abort.
	UScriptableGraphSubsystem* Subsystem = UScriptableGraphSubsystem::Get(InOwner);
	if (!Subsystem) return;
	Subsystem->RegisterRunner(this);

	// Populate the runtime context bag with the values from the external context.
	Context.MigrateToNewBagInstance(InContext.GetBag());

	// Deep-copy every node from the asset. Parent to this runner so they share its lifetime.
	Nodes.Reserve(Asset->Nodes.Num());
	NodesByID.Reserve(Asset->Nodes.Num());
	for (const TObjectPtr<UScriptableNode>& AssetNode : Asset->Nodes)
	{
		if (!AssetNode) continue;

		UScriptableNode* NodeCopy = DuplicateObject<UScriptableNode>(AssetNode, this);
		if (!NodeCopy) continue;

		Nodes.Add(NodeCopy);
		NodesByID.Add(NodeCopy->GetBindingID(), NodeCopy);

		// Register the node's binding proxy (e.g. its task) so other nodes can read its Output.
		// IDs survive DuplicateObject, so they match the SourcePaths baked at edit time.
		if (UScriptableObject* Proxy = NodeCopy->GetBindingProxy())
		{
			NodeBindingMap.Add(Proxy->GetBindingID(), Proxy);
		}
	}

	// Cache the Exit node, if any. First one wins; the validator flags graphs with more than one.
	for (const TObjectPtr<UScriptableNode>& Node : Nodes)
	{
		if (UScriptableNode_Exit* Exit = Cast<UScriptableNode_Exit>(Node))
		{
			ExitNode = Exit;
			break;
		}
	}

	// Build the connection lookup once. Reads from the asset's immutable connection list.
	for (const FScriptableGraphConnection& Conn : Asset->Connections)
	{
		OutputToInputs.Add(Conn.From, Conn.To);
	}

	// Wire runtime data and subscribe to every node's pin/inactive notifications.
	for (const TObjectPtr<UScriptableNode>& Node : Nodes)
	{
		if (!Node) continue;

		// Nodes resolve against the graph context and, via NodeBindingMap, against other nodes' Outputs.
		// Cross-node reads resolve at activation time (UScriptableTask::Begin), so an Output is already
		// populated when a downstream node reads it; ordering across event paths is the author's concern.
		Node->InitRuntimeData(&Context, &NodeBindingMap);
		Node->Register(Owner);

		Node->OnPinFiredNative.AddUObject(this, &UScriptableGraphInstance::HandleNodePinFired);
		Node->OnNodeInactiveNative.AddUObject(this, &UScriptableGraphInstance::HandleNodeInactive);
		Node->OnRequestEventNative.AddUObject(this, &UScriptableGraphInstance::HandleNodeRequestEvent);
		Node->OnRequestFinishGraphNative.AddUObject(this, &UScriptableGraphInstance::HandleNodeRequestFinishGraph);
	}

	// Activate the Entry node. Its synchronous Activate() will mark Out, fire it, and propagate.
	if (const TObjectPtr<UScriptableNode>* EntryPtr = NodesByID.Find(Asset->EntryNodeID))
	{
		if (UScriptableNode* Entry = *EntryPtr)
		{
			ActiveNodes.Add(Entry);

			// Entry is always a UScriptableNode_Entry. Calling Activate() fires its only output synchronously,
			// which feeds HandleNodePinFired, which enqueues downstream activations and starts the drain.
			if (UScriptableNode_Entry* TypedEntry = Cast<UScriptableNode_Entry>(Entry))
			{
				TypedEntry->Activate();
			}
		}
	}

	// Activate() above may have left the queue with pending activations. Drain now (if not already).
	if (!bProcessing)
	{
		ProcessQueue();
	}
}

void UScriptableGraphInstance::FireEvent(FName EventName)
{
	if (EventName.IsNone()) return;
	if (bCancelled || bFinished) return;

	// Linear scan for now.
	for (const TObjectPtr<UScriptableNode>& Node : Nodes)
	{
		UScriptableNode_ReceiveEvent* Receiver = Cast<UScriptableNode_ReceiveEvent>(Node);
		if (!Receiver || Receiver->EventName != EventName) continue;

		ActiveNodes.Add(Receiver);
		Receiver->Trigger();
	}

	if (!bProcessing)
	{
		ProcessQueue();
	}
}

void UScriptableGraphInstance::Cancel()
{
	if (bCancelled || bFinished) return;

	// With an unfired Exit node: run only its Cancelled cleanup sub-flow. Abort the in-flight normal
	// nodes and drop their pending activations first so the cancel takes effect immediately, then fire
	// "Cancelled". We intentionally do NOT set bCancelled here — that would block ProcessQueue from
	// draining the cleanup. Once the cleanup empties, CheckCompletion runs Finish (bExitTriggered is set,
	// so it won't re-fire the Exit).
	if (!bExitTriggered && ExitNode)
	{
		bExitTriggered = true;
		Pending.Empty();
		TeardownNodes();
		HandleNodePinFired(ExitNode, UScriptableNode_Exit::CancelledOutputName);
		return;
	}

	// No Exit (or it already fired): cancelled graphs never propagate further — tear down now.
	bCancelled = true;
	Pending.Empty();
	TeardownNodes();
	Finish();
}

void UScriptableGraphInstance::CancelImmediate()
{
	if (bCancelled || bFinished) return;

	// World teardown path: the world and any actors the Exit sub-flow might touch are being destroyed,
	// so skip the Exit entirely and tear down aggressively (this is the pre-Exit Cancel behavior).
	bCancelled = true;
	Pending.Empty();
	TeardownNodes();
	Finish();
}

void UScriptableGraphInstance::ReplaceContext(const FScriptableContext& NewContext)
{
	Context.MigrateToNewBagInstance(NewContext.GetBag());
}

void UScriptableGraphInstance::HandleNodePinFired(UScriptableNode* Node, FName OutputName)
{
	if (bCancelled || !Node) return;

	// Capture Exit's first fire for parent SubGraph routing.
	if (Node == ExitNode && CompletionOutput.IsNone())
	{
		CompletionOutput = OutputName;
	}

	const FScriptableGraphPinRef OutputRef{ Node->GetBindingID(), OutputName };

	// Enqueue every downstream input wired to this output.
	TArray<FScriptableGraphPinRef> Targets;
	OutputToInputs.MultiFind(OutputRef, Targets);

	for (const FScriptableGraphPinRef& Target : Targets)
	{
		if (const TObjectPtr<UScriptableNode>* DestPtr = NodesByID.Find(Target.NodeID))
		{
			if (UScriptableNode* Dest = *DestPtr)
			{
				Pending.Add({ Dest, Target.PinName });
			}
		}
	}

	// Kick the drain only if we're not already inside it. Re-entrant calls from within ProcessQueue
	// simply append to Pending and the outer loop picks them up on the next iteration.
	if (!bProcessing)
	{
		ProcessQueue();
	}
}

void UScriptableGraphInstance::HandleNodeInactive(UScriptableNode* Node)
{
	if (!Node) return;

	ActiveNodes.Remove(Node);

	// If a latent node becomes inactive outside the drain (e.g. the last living node was waiting and
	// just notified inactivity without firing any pin), CheckCompletion needs to run here.
	if (!bProcessing)
	{
		CheckCompletion();
	}
}

void UScriptableGraphInstance::HandleNodeRequestEvent(FName EventName)
{
	if (bCancelled || bFinished) return;

	// A Go To node asked us to jump. FireEvent is re-entrant: inside the drain it just enqueues the
	// target ReceiveEvent's downstream activations, which the running ProcessQueue then picks up.
	FireEvent(EventName);
}

void UScriptableGraphInstance::HandleNodeRequestFinishGraph(FName OutputName)
{
	if (bCancelled || bFinished || bExitTriggered) return;

	// First Finish wins. Stop in-flight nodes (no pin propagation), clear pending, route via Exit.
	bExitTriggered = true;
	CompletionOutput = OutputName;
	Pending.Empty();
	TeardownNodes();

	if (ExitNode)
	{
		HandleNodePinFired(ExitNode, OutputName);
	}
	else
	{
		Finish();
	}
}

void UScriptableGraphInstance::ProcessQueue()
{
	if (bProcessing || bCancelled) return;

	bProcessing = true;

	while (!Pending.IsEmpty() && !bCancelled)
	{
		const FPendingActivation Activation = Pending[0];
		Pending.RemoveAt(0);

		if (!Activation.Node) continue;

		if (!ActiveNodes.Contains(Activation.Node))
		{
			ActiveNodes.Add(Activation.Node);
		}

		Activation.Node->ActivateInput(Activation.InputName);
	}

	bProcessing = false;

	CheckCompletion();
}

void UScriptableGraphInstance::CheckCompletion()
{
	if (bFinished || bCancelled) return;
	if (!ActiveNodes.IsEmpty()) return;
	if (!Pending.IsEmpty()) return;

	// Natural completion: run the Exit's cleanup sub-flow once before finishing. Firing "Finished"
	// enqueues its downstream; the queue drains and we return here with bExitTriggered set, then Finish.
	if (!bExitTriggered && ExitNode)
	{
		bExitTriggered = true;
		HandleNodePinFired(ExitNode, UScriptableNode_Exit::FinishedOutputName);
		return;
	}

	Finish();
}

void UScriptableGraphInstance::TeardownNodes()
{
	for (const TObjectPtr<UScriptableNode>& Node : ActiveNodes)
	{
		if (!Node) continue;

		// Unsubscribe before tearing down so downstream notifications stay quiet.
		Node->OnPinFiredNative.RemoveAll(this);
		Node->OnNodeInactiveNative.RemoveAll(this);
		Node->OnRequestEventNative.RemoveAll(this);
		Node->OnRequestFinishGraphNative.RemoveAll(this);

		Node->Teardown();
		Node->Unregister();
	}

	ActiveNodes.Empty();
}

void UScriptableGraphInstance::Finish()
{
	if (bFinished) return;

	bFinished = true;

	// Unsubscribe from any remaining (non-active) nodes so dangling broadcasts don't reach us.
	for (const TObjectPtr<UScriptableNode>& Node : Nodes)
	{
		if (!Node) continue;
		Node->OnPinFiredNative.RemoveAll(this);
		Node->OnNodeInactiveNative.RemoveAll(this);
		Node->OnRequestEventNative.RemoveAll(this);
		Node->OnRequestFinishGraphNative.RemoveAll(this);
	}

	OnGraphFinishedNative.Broadcast();

	// Unregister from the subsystem; once it drops its strong ref we become collectible. If the world
	// is already tearing down the subsystem may be gone — that's fine, it already cancelled us.
	if (UScriptableGraphSubsystem* Subsystem = UScriptableGraphSubsystem::Get(Owner))
	{
		Subsystem->UnregisterRunner(this);
	}
}

void UScriptableGraphInstance::BeginDestroy()
{
	// If the runner is being destroyed mid-flight (owner gone, world tearing down), tear down silently.
	if (!bFinished && !bCancelled)
	{
		bCancelled = true;
		Pending.Empty();
		TeardownNodes();
	}

	// Defensive: drop ourselves from the subsystem if we somehow reach GC still registered.
	if (UScriptableGraphSubsystem* Subsystem = UScriptableGraphSubsystem::Get(Owner))
	{
		Subsystem->UnregisterRunner(this);
	}

	Super::BeginDestroy();
}
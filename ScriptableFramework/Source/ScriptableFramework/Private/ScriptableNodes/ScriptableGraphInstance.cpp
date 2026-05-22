// Copyright 2026 kirzo

#include "ScriptableNodes/ScriptableGraphInstance.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableNodes/ScriptableNode_Entry.h"
#include "ScriptableContext.h"

void UScriptableGraphInstance::Launch(UScriptableGraph* InAsset, UObject* InOwner, const FScriptableContext& InContext)
{
	if (!InAsset || !InOwner) return;

	Asset = InAsset;
	Owner = InOwner;

	// Keep the runner alive until the graph finishes or is cancelled.
	SelfReference = this;

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

		// Nodes can only resolve bindings against the graph context. They cannot bind to each other
		// because the graph's execution order is not guaranteed at design time.
		Node->InitRuntimeData(&Context, nullptr);
		Node->Register(Owner);

		Node->OnPinFiredNative.AddUObject(this, &UScriptableGraphInstance::HandleNodePinFired);
		Node->OnNodeInactiveNative.AddUObject(this, &UScriptableGraphInstance::HandleNodeInactive);
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

void UScriptableGraphInstance::Cancel()
{
	if (bCancelled || bFinished) return;

	bCancelled = true;

	// Drop anything queued; cancelled graphs never propagate further.
	Pending.Empty();

	TeardownNodes();
	Finish();
}

void UScriptableGraphInstance::HandleNodePinFired(UScriptableNode* Node, FName OutputName)
{
	if (bCancelled || !Node) return;

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
	}

	OnGraphFinishedNative.Broadcast();

	// Release the GC anchor. Next GC sweep collects us.
	SelfReference = nullptr;
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

	Super::BeginDestroy();
}
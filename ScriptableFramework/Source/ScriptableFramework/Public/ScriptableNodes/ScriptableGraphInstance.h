// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableNodes/ScriptableGraph.h"
#include "ScriptableNodes/ScriptableGraphConnection.h"
#include "StructUtils/PropertyBag.h"
#include "Core/KzPropertyBagHelpers.h"
#include "ScriptableGraphInstance.generated.h"

class UScriptableObject;
class UScriptableNode;
class UScriptableNode_Exit;
class UScriptableGraphSubsystem;
struct FScriptableContext;

DECLARE_MULTICAST_DELEGATE(FScriptableGraphFinishedNative);

/**
 * Runtime owner for a single execution of a UScriptableGraph (created by UScriptableGraph::Run).
 * Deep-copies the asset's nodes, reads its connections as immutable lookup data, pumps activations
 * through the pin-based model, and tears itself down when no active work remains.
 */
UCLASS(BlueprintType)
class SCRIPTABLEFRAMEWORK_API UScriptableGraphInstance : public UObject
{
	GENERATED_BODY()

public:
	/** Sets up runtime state from the asset and starts execution by activating the Entry node. */
	void Launch(UScriptableGraph* InAsset, UObject* InOwner, const FScriptableContext& InContext);

	/** Wakes every matching UScriptableNode_ReceiveEvent (parallel fan-out; no match = no-op). Re-entrant: appends to the in-flight drain if one is running, else starts a new one. Game-thread only. */
	UFUNCTION(BlueprintCallable, Category = "Scriptable Framework|Graph")
	void FireEvent(FName EventName);

	/** Hard-cancels the graph: tears down active nodes in-place, drops queued activations, unregisters from the subsystem. */
	UFUNCTION(BlueprintCallable, Category = "Scriptable Framework|Graph")
	void Cancel();

	/** Mutates a context value in place. */
	template <typename T>
	void SetContextValue(const FName& Name, const T& Value)
	{
		KzPropertyBag::Set(Context, Name, Value);
	}

	/** Replace the entire context bag with NewContext's bag. */
	UFUNCTION(BlueprintCallable, Category = "Scriptable Framework")
	void ReplaceContext(const FScriptableContext& NewContext);

	/** Read-only access to the live context bag. */
	const FInstancedPropertyBag& GetContextBag() const { return Context; }

	/** Mutable access to the live context bag. */
	FInstancedPropertyBag& GetMutableContextBag() { return Context; }

	/** Returns true if execution is still in progress (active nodes or pending activations exist). */
	UFUNCTION(BlueprintCallable, Category = "Scriptable Framework|Graph")
	bool IsRunning() const { return !ActiveNodes.IsEmpty() || !Pending.IsEmpty(); }

	/** Returns true if execution has finished. */
	UFUNCTION(BlueprintCallable, Category = "Scriptable Framework|Graph")
	bool IsFinished() const { return bFinished; }

	/** Returns true if execution was cancelled. */
	UFUNCTION(BlueprintCallable, Category = "Scriptable Framework|Graph")
	bool IsCancelled() const { return bCancelled; }

	/** Source graph asset. */
	UScriptableGraph* GetAsset() const { return Asset; }

	/** Object that requested the run. Used by the runner viewer to surface "who's running this graph". */
	UObject* GetOwner() const { return Owner; }

	/** Read-only view of the nodes currently running. Used by the runner viewer to list and the canvas overlay to highlight. */
	const TSet<TObjectPtr<UScriptableNode>>& GetActiveNodes() const { return ActiveNodes; }

	/** Every duplicated node this runner owns. Used by the viewer to subscribe to OnPinFiredNative on each. */
	const TArray<TObjectPtr<UScriptableNode>>& GetNodes() const { return Nodes; }

	/**
	 * Exit output captured at completion (set when Exit fires or a Finish requests).
	 * NAME_None until committed.
	 * SubGraph nodes read this on OnGraphFinishedNative to route the parent pin.
	 */
	FName GetCompletionOutput() const { return CompletionOutput; }

	/** Broadcast once when the graph naturally finishes (no active nodes, no pending activations). */
	FScriptableGraphFinishedNative OnGraphFinishedNative;

	virtual void BeginDestroy() override;

private:
	struct FPendingActivation
	{
		TObjectPtr<UScriptableNode> Node;
		FName InputName;
	};

	void HandleNodePinFired(UScriptableNode* Node, FName OutputName);
	void HandleNodeInactive(UScriptableNode* Node);
	void HandleNodeRequestEvent(FName EventName);

	/**
	 * First-Finish-wins: stop active nodes (no pin propagation) and route through
	 * Exit's OutputName so the cleanup sub-flow runs the right branch. Re-entry ignored.
	 */
	void HandleNodeRequestFinishGraph(FName OutputName);

	void ProcessQueue();
	void CheckCompletion();
	void TeardownNodes();
	void Finish();

	/** Subsystem-driven cancel during world teardown: skips the Exit cleanup sub-flow and tears down immediately. */
	void CancelImmediate();

	/** Source asset. Connections are read directly from here, not copied. */
	UPROPERTY()
	TObjectPtr<UScriptableGraph> Asset;

	/** Owner provided to nodes when they Register. */
	UPROPERTY()
	TObjectPtr<UObject> Owner;

	/** Deep-copies of the asset's nodes, parented to this runner. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UScriptableNode>> Nodes;

	/** Runtime context bag populated from the external FScriptableContext at Launch. */
	UPROPERTY(Transient)
	FInstancedPropertyBag Context;

	/** Output -> Inputs lookup built once in Launch. Iterated when a node fires an output. */
	TMultiMap<FScriptableGraphPinRef, FScriptableGraphPinRef> OutputToInputs;

	/** Fast lookup from ID to the deep-copied node. */
	TMap<FGuid, TObjectPtr<UScriptableNode>> NodesByID;

	/** Binding sources keyed by binding ID: each node's proxy (e.g. its task), so a node's Input can read another node's Output. Injected into every node via InitRuntimeData. */
	UPROPERTY(Transient)
	TMap<FGuid, TObjectPtr<UScriptableObject>> NodeBindingMap;

	/** Nodes with at least one active pin. */
	TSet<TObjectPtr<UScriptableNode>> ActiveNodes;

	/** Cached Exit node if the asset declares one. Null when the graph has no Exit. */
	UPROPERTY(Transient)
	TObjectPtr<UScriptableNode_Exit> ExitNode;

	/** True once the Exit's Finished/Cancelled output has been fired. Prevents re-triggering it. */
	bool bExitTriggered = false;

	/**
	 * Captured Exit output name. Exposed via GetCompletionOutput so parent SubGraph
	 * nodes know which pin to fire when OnGraphFinishedNative broadcasts.
	 */
	FName CompletionOutput = NAME_None;

	/** FIFO of pending input activations. Drained by ProcessQueue. */
	TArray<FPendingActivation> Pending;

	bool bProcessing = false;
	bool bCancelled = false;
	bool bFinished = false;

	/** The subsystem owns the live-runner registry and calls Register/Unregister on us. */
	friend class UScriptableGraphSubsystem;
};
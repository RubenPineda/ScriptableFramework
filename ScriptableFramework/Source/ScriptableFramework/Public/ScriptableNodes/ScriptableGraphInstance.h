// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableNodes/ScriptableGraph.h"
#include "ScriptableNodes/ScriptableGraphConnection.h"
#include "StructUtils/PropertyBag.h"
#include "ScriptableGraphInstance.generated.h"

class UScriptableNode;
struct FScriptableContext;

DECLARE_MULTICAST_DELEGATE(FScriptableGraphFinishedNative);

/**
 * Runtime owner for a single execution of a UScriptableGraph.
 * UScriptableGraph::Run(...) constructs and launches an instance under the hood.
 *
 * Deep-copies the asset's nodes, references the asset's connections as immutable lookup data,
 * pumps activations through the pin-based execution model, and tears itself down when no active
 * work remains.
 */
UCLASS()
class UScriptableGraphInstance : public UObject
{
	GENERATED_BODY()

public:
	/** Sets up runtime state from the asset and starts execution by activating the Entry node. */
	void Launch(UScriptableGraph* InAsset, UObject* InOwner, const FScriptableContext& InContext);

	/** Hard-cancels the graph: tears down active nodes in-place, drops queued activations, releases self. */
	UFUNCTION(BlueprintCallable, Category = "Scriptable Framework|Graph")
	void Cancel();

	/** Returns true if execution is still in progress (active nodes or pending activations exist). */
	UFUNCTION(BlueprintCallable, Category = "Scriptable Framework|Graph")
	bool IsRunning() const { return !ActiveNodes.IsEmpty() || !Pending.IsEmpty(); }

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

	void ProcessQueue();
	void CheckCompletion();
	void TeardownNodes();
	void Finish();

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

	/** Nodes with at least one active pin. */
	TSet<TObjectPtr<UScriptableNode>> ActiveNodes;

	/** FIFO of pending input activations. Drained by ProcessQueue. */
	TArray<FPendingActivation> Pending;

	/** Self-reference: keeps this runner alive across GC while execution is in flight. */
	UPROPERTY()
	TObjectPtr<UScriptableGraphInstance> SelfReference;

	bool bProcessing = false;
	bool bCancelled = false;
	bool bFinished = false;
};
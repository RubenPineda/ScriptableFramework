// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableObject.h"
#include "ScriptableNode.generated.h"

class UScriptableNode;

DECLARE_MULTICAST_DELEGATE_TwoParams(FScriptableNodePinFiredNative, UScriptableNode* /*Node*/, FName /*OutputName*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FScriptableNodeInactiveNative, UScriptableNode* /*Node*/);

/**
 * Base class for any node that can live inside a UScriptableGraph.
 * Defines the pin-state model (active input/output pins) and the activation protocol used by the runner.
 *
 * Subclasses declare their inputs/outputs, decide when an active input can be consumed (CanProcessInput),
 * and implement the actual reaction to consumption (ProcessInput). The base class manages pin sets,
 * fires the appropriate delegates, and notifies the runner when the node becomes inactive.
 */
UCLASS(Abstract, DefaultToInstanced, EditInlineNew, Blueprintable, BlueprintType, HideCategories = (Hidden), CollapseCategories)
class SCRIPTABLEFRAMEWORK_API UScriptableNode : public UScriptableObject
{
	GENERATED_BODY()

public:
	/** Canonical name of the implicit Stop input, declared by subclasses that support cancellation. */
	static const FName StopInputName;

	/** Canonical name of the implicit Stopped output, auto-generated when Stop is declared as an input. */
	static const FName StoppedOutputName;

	/** Nodes are not expected to tick by themselves. */
	virtual bool CanEverTick() const final override { return false; }

	/** Returns the declared input pins of this node. */
	virtual TArray<FName> GetInputPins() const { return {}; }

	/** Returns the outputs the subclass explicitly declares. */
	virtual TArray<FName> GetDeclaredOutputPins() const { return {}; }

	/*** Returns the effective output pins of this node. */
	TArray<FName> GetOutputPins() const;

	/** Returns true if the given input is ready to be processed right now. */
	virtual bool CanProcessInput(FName InputName) const { return true; }

	/**
	 * External activation entry point. Called by the runner when a wire delivers a signal to InputName.
	 * Marks the input active and, if CanProcessInput returns true, invokes ProcessInput.
	 */
	void ActivateInput(FName InputName);

	/** Returns true if any input or output pin is currently active. */
	bool IsNodeActive() const { return !ActiveInputPins.IsEmpty() || !ActiveOutputPins.IsEmpty(); }

	/** Read-only view of the currently active input pins. */
	const TSet<FName>& GetActiveInputPins() const { return ActiveInputPins; }

	/** Read-only view of the currently active output pins. */
	const TSet<FName>& GetActiveOutputPins() const { return ActiveOutputPins; }

	/** Fired when an output pin is fired and propagated. The runner subscribes to follow wires. */
	FScriptableNodePinFiredNative OnPinFiredNative;

	/** Fired when the node has no active pins left. The runner subscribes to detect node completion. */
	FScriptableNodeInactiveNative OnNodeInactiveNative;

	/**
	 * Releases any in-flight resources without propagating downstream. Called by the graph runner
	 * when the graph is hard-cancelled. Subclasses override to e.g. cancel internal tasks silently.
	 */
	virtual void Teardown() {}

protected:
	/**
	 * Implements the reaction to an active input being consumed. Default: no-op.
	 * Subclasses are responsible for calling MarkInputInactive on the input they consume.
	 */
	virtual void ProcessInput(FName InputName) {}

	/** Marks the given input as inactive. Subclasses call this when they consume an input. */
	void MarkInputInactive(FName InputName);

	/** Marks the given output as active. Subclasses call this when they intend to fire an output later. */
	void MarkOutputActive(FName OutputName);

	/** Marks the given output as inactive without firing. Use for cleanup paths (e.g. Stop invalidating pending outputs). */
	void MarkOutputInactive(FName OutputName);

	/**
	 * Fires the given output: broadcasts the pin-fired delegate so the runner can propagate, then
	 * marks the output inactive. If this leaves the node with no active pins, notifies inactivity.
	 */
	void FireOutput(FName OutputName);

	/** Helper to deactivate every currently active output without firing. Used in cancellation paths. */
	void DeactivateAllOutputs();

private:
	/** Checks whether the node just became inactive and broadcasts OnNodeInactiveNative if so. */
	void NotifyIfInactive();

	UPROPERTY(Transient)
	TSet<FName> ActiveInputPins;

	UPROPERTY(Transient)
	TSet<FName> ActiveOutputPins;
};
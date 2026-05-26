// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableNode_AND.generated.h"

/**
 * Joins N input pulses into a single "Out". Fires once all declared inputs have pulsed at least once
 * since the last fire, then resets. Pulses accumulate across inputs in any order and across frames
 * (latent upstreams welcome); repeated pulses on the same input are idempotent. Removing an input via
 * the canvas also drops it from the seen-set so the gate can't deadlock. Matches the K2/Blueprint AND.
 */
UCLASS(DisplayName = "AND", meta = (NodeCategory = "System|Flow"))
class SCRIPTABLEFRAMEWORK_API UScriptableNode_AND : public UScriptableNode
{
	GENERATED_BODY()

public:
	/** Number of input pins. Minimum 2; an AND with one input is a no-op pass-through. */
	UPROPERTY()
	int32 InputCount = 2;

	/** Hard floor for input count guards across pin manipulation paths. */
	static constexpr int32 MinInputCount = 2;

	/** Canonical output pin name. */
	static const FName OutOutputName;

	/** Builds an input pin name from the branch index ("0", "1", ...), matching Sequence's numbering. */
	static FName MakeInputName(int32 BranchIndex);

	//~ UScriptableNode interface
	virtual TArray<FName> GetInputPins() const override;
	virtual TArray<FName> GetDeclaredOutputPins() const override;
	//~ End of UScriptableNode interface

#if WITH_EDITOR
	virtual FText GetDisplayTitle() const override;

	/** Editor helper: bumps InputCount by one. Mirrors UScriptableNode_Sequence::AddOutputPin. */
	void AddInputPin();

	/** Editor helper: removes the input pin at BranchIndex, shifting higher pins down and rewriting incoming wires so connections stay intact. */
	void RemoveInputPinAt(int32 BranchIndex);
#endif

protected:
	//~ UScriptableNode interface
	virtual void ProcessInput(FName InputName) override;
	//~ End of UScriptableNode interface

private:
	/** Inputs that have pulsed since the last fire. Transient runtime state; reset on each fire. */
	UPROPERTY(Transient)
	TSet<FName> SeenInputs;
};
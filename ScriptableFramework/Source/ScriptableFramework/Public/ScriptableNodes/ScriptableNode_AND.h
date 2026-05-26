// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableNode_AND.generated.h"

/**
 * Joins N input pulses into a single output. Fires "Out" once all declared inputs have been
 * activated at least once since the last fire. Pulses across different inputs accumulate, so
 * inputs may arrive in any order and across multiple frames (latent upstreams welcome). After
 * firing, the gate resets and is ready to gather the next round of pulses.
 *
 * Within a single activation cycle, repeated pulses on the same input are idempotent (the set
 * of "seen" inputs is a set, not a counter) — matching the K2/blueprint AND convention. Inputs
 * removed via the canvas affordance never deadlock the gate because pruning also drops their
 * entry from the seen-set.
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

	/** Builds an input pin name for the given branch index ("0", "1", ...). Same convention as Sequence so the user sees a consistent numbering across native flow nodes. */
	static FName MakeInputName(int32 BranchIndex);

	//~ UScriptableNode interface
	virtual TArray<FName> GetInputPins() const override;
	virtual TArray<FName> GetDeclaredOutputPins() const override;
	//~ End of UScriptableNode interface

#if WITH_EDITOR
	virtual FText GetDisplayTitle() const override;

	/** Editor helper: bumps InputCount by one. Mirrors UScriptableNode_Sequence::AddOutputPin. */
	void AddInputPin();

	/** Editor helper: removes the input pin at BranchIndex, shifting higher-indexed pins down by one and rewriting incoming wires in the owning graph so connections keep pointing at the same branch under its new name. */
	void RemoveInputPinAt(int32 BranchIndex);
#endif

protected:
	//~ UScriptableNode interface
	virtual void ProcessInput(FName InputName) override;
	//~ End of UScriptableNode interface

private:
	/** Inputs that have already pulsed since the last fire. Transient: belongs to the runtime cycle, not to the asset's persisted state. Reset on each fire. */
	UPROPERTY(Transient)
	TSet<FName> SeenInputs;
};
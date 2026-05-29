// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableNode_OR.generated.h"

/**
 * Fires "Out" on the first input pulse and ignores every subsequent pulse for the lifetime of the
 * node. Once fired the node never re-arms — additional pulses are dropped silently and the inputs
 * are not even momentarily marked active. Use for one-shot "first wins" merging.
 */
UCLASS(DisplayName = "OR", meta = (NodeCategory = "System|Flow"))
class SCRIPTABLEFRAMEWORK_API UScriptableNode_OR : public UScriptableNode
{
	GENERATED_BODY()

public:
	/** Number of input pins. Minimum 2; one input is a degenerate pass-through. */
	UPROPERTY()
	int32 InputCount = 2;

	/** Hard floor for input count guards across pin manipulation paths. */
	static constexpr int32 MinInputCount = 2;

	/** Canonical output pin name. */
	static const FName OutOutputName;

	/** Builds an input pin name from the branch index ("0", "1", ...), matching AND's numbering. */
	static FName MakeInputName(int32 BranchIndex);

	//~ UScriptableNode interface
	virtual TArray<FName> GetInputPins() const override;
	virtual TArray<FName> GetDeclaredOutputPins() const override;
	//~ End of UScriptableNode interface

#if WITH_EDITOR
	virtual FText GetDisplayTitle() const override;

	/** Editor helper: bumps InputCount by one. Mirrors UScriptableNode_AND::AddInputPin. */
	void AddInputPin();

	/** Editor helper: removes the input pin at BranchIndex, shifting higher pins down and rewriting incoming wires so connections stay intact. */
	void RemoveInputPinAt(int32 BranchIndex);
#endif

protected:
	//~ UScriptableNode interface
	virtual void ProcessInput(FName InputName) override;
	//~ End of UScriptableNode interface

private:
	/** True once Out has fired. While set, further input pulses are short-circuited: ProcessInput
	 * marks the input inactive immediately and returns, so the OR is permanently latched and no
	 * downstream observer ever sees the late pulse. */
	UPROPERTY(Transient)
	bool bFired = false;
};

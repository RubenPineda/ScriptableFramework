// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableNode_Sequence.generated.h"

/**
 * Fans the incoming signal out across N ordered outputs, firing them in order within one ProcessInput
 * (each fire just queues downstream work; no branch waits for another). Mirrors the Blueprint Sequence node.
 */
UCLASS(DisplayName = "Sequence", meta = (NodeCategory = "System|Flow"))
class SCRIPTABLEFRAMEWORK_API UScriptableNode_Sequence : public UScriptableNode
{
	GENERATED_BODY()

public:
	/** Hard floor for output count, enforced in pin-removal and enumeration paths. */
	static constexpr int32 MinOutputCount = 2;

	/** Number of ordered outputs. Default 2 (BP-style). */
	UPROPERTY()
	int32 OutputCount = 2;

	/** Canonical name of the input pin. */
	static const FName InInputName;

	/** Builds an output pin name for the given branch index ("Then 0", "Then 1", ...). */
	static FName MakeOutputName(int32 BranchIndex);

	//~ UScriptableNode interface
	virtual TArray<FName> GetInputPins() const override;
	virtual TArray<FName> GetDeclaredOutputPins() const override;
	//~ End of UScriptableNode interface

#if WITH_EDITOR
	/** Editor helper: adds one output pin. */
	void AddOutputPin();

	/** Editor helper: removes the output pin at BranchIndex and shifts higher pins down, rewriting the owning graph's wires so downstream stays connected. Wires from the removed pin are lost. No-op if only one pin would remain or index is out of range. */
	void RemoveOutputPinAt(int32 BranchIndex);
#endif

protected:
	//~ UScriptableNode interface
	virtual void ProcessInput(FName InputName) override;
	//~ End of UScriptableNode interface
};
// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableNode_Sequence.generated.h"

/**
 * Fans the incoming signal out across N ordered outputs ("Then 0", "Then 1", ...).
 * Outputs fire synchronously in order during a single ProcessInput call: each FireOutput broadcasts
 * to the runner, which queues downstream activations on Pending; no branch waits for any other to
 * complete. Mirrors the Blueprint Sequence node semantics.
 */
UCLASS(DisplayName = "Sequence", meta = (NodeCategory = "System|Flow"))
class SCRIPTABLEFRAMEWORK_API UScriptableNode_Sequence : public UScriptableNode
{
	GENERATED_BODY()

public:
	/** Minimum number of output pins. Hard floor for guards in pin removal paths and defensive clamps in pin enumeration. */
	static constexpr int32 MinOutputCount = 2;

	/** Number of ordered outputs the node exposes. BP-style default is 2; minimum 1 for placeholder / pass-through uses. */
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
	/** Editor helper: bumps OutputCount by one. Emits a property-changed event so the editor's title/pin refresh paths fire automatically (matches the user editing OutputCount in the details panel). */
	void AddOutputPin();

	/** Editor helper: removes the output pin at BranchIndex (0-based) and shifts every higher-indexed pin down by one, rewriting the outgoing wires in the owning UScriptableGraph so downstream connections keep pointing at the same branch under its new name. Connections originating from the victim pin are lost; connections originating from any other branch slide down their indices intact. No-op when only one pin would remain or BranchIndex is out of range. */
	void RemoveOutputPinAt(int32 BranchIndex);
#endif

protected:
	//~ UScriptableNode interface
	virtual void ProcessInput(FName InputName) override;
	//~ End of UScriptableNode interface
};
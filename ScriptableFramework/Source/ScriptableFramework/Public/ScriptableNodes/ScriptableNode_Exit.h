// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableNode_Exit.generated.h"

/**
 * Optional cleanup endpoint. The runner fires one of its outputs right before the graph finishes,
 * so a graph can run a guaranteed teardown sub-flow (close UI, release handles, notify listeners).
 * No inputs (not reachable by normal flow) — the runner triggers it directly. At most one per graph.
 */
UCLASS(DisplayName = "Exit", meta = (NodeCategory = "System|Flow"))
class SCRIPTABLEFRAMEWORK_API UScriptableNode_Exit : public UScriptableNode
{
	GENERATED_BODY()

public:
	/** Fired on natural completion (the queue drained without a Cancel). */
	static const FName FinishedOutputName;

	/** Fired on a manual Cancel() of the runner. */
	static const FName CancelledOutputName;

	//~ UScriptableNode interface
	virtual TArray<FName> GetInputPins() const override { return {}; }
	virtual TArray<FName> GetDeclaredOutputPins() const override;
	//~ End of UScriptableNode interface
};

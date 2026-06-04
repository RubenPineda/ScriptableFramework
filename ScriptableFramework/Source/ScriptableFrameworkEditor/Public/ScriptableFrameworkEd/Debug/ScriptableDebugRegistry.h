// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"

class UScriptableGraph;
class UScriptableGraphInstance;

/**
 * Per-asset "currently focused" runner. The graph editor's Debug Object combo writes here; the
 * SScriptableGraphNode_Base overlay reads here to decide whether to halo an active node. Decoupled
 * from the toolkit so Slate widgets don't need a toolkit handle. Weak refs auto-clear on instance
 * teardown so the combo and overlay drop gracefully without explicit cleanup.
 */
namespace FScriptableDebugRegistry
{
	SCRIPTABLEFRAMEWORKEDITOR_API void SetDebugInstance(const UScriptableGraph* Asset, UScriptableGraphInstance* Instance);

	/** Returns the chosen debug instance for an asset, or null if none / instance died. */
	SCRIPTABLEFRAMEWORKEDITOR_API UScriptableGraphInstance* GetDebugInstance(const UScriptableGraph* Asset);
}

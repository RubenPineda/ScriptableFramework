// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"

class UScriptableNode;
class UScriptableEdGraphNode;

/**
 * Lookup table: runtime UScriptableNode class -> visual UScriptableEdGraphNode class. Populated
 * once at module startup by scanning every subclass of UScriptableEdGraphNode.
 */
class SCRIPTABLEFRAMEWORKEDITOR_API FScriptableEdGraphNodeRegistry
{
public:
	/** Populates the singleton by iterating all UScriptableEdGraphNode subclasses. Idempotent. */
	static void Build();

	/** Returns the ed-node UClass registered for the given runtime instance, or null if none — caller should fall back to the generic native ed-node. */
	static UClass* FindEdNodeClassFor(const UScriptableNode* RuntimeNode);

private:
	static TMap<UClass*, UClass*>& GetMap();
};
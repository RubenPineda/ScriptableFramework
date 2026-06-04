// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "ScriptableEdGraph.generated.h"

/**
 * Editor-only visual representation of a UScriptableGraph asset.
 * Holds UEdGraphNodes that mirror the asset's runtime UScriptableNode list.
 */
UCLASS()
class UScriptableEdGraph : public UEdGraph
{
	GENERATED_BODY()

public:
	/** Currently-selected ed-nodes. Transient — owned by the host editor's selection callback, read by the connection drawing policy to highlight incident wires. */
	TSet<const UEdGraphNode*> SelectedNodes;
};
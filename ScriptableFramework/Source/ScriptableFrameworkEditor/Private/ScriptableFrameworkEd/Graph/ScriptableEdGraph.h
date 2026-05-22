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
};
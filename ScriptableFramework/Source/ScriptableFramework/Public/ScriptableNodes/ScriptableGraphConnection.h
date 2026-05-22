// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableGraphConnection.generated.h"

/** Reference to a single pin (input or output) on a specific node within a graph. */
USTRUCT()
struct SCRIPTABLEFRAMEWORK_API FScriptableGraphPinRef
{
	GENERATED_BODY()

	/** Persistent ID of the node holding this pin. */
	UPROPERTY()
	FGuid NodeID;

	/** Name of the pin within the node. */
	UPROPERTY()
	FName PinName;

	bool operator==(const FScriptableGraphPinRef& Other) const
	{
		return NodeID == Other.NodeID && PinName == Other.PinName;
	}

	bool operator!=(const FScriptableGraphPinRef& Other) const { return !(*this == Other); }

	friend uint32 GetTypeHash(const FScriptableGraphPinRef& Ref)
	{
		return HashCombine(GetTypeHash(Ref.NodeID), GetTypeHash(Ref.PinName));
	}
};

/**
 * A single directed connection between two pins: From (output) feeds To (input).
 * Stored centrally in UScriptableGraph::Connections.
 */
USTRUCT()
struct SCRIPTABLEFRAMEWORK_API FScriptableGraphConnection
{
	GENERATED_BODY()

	/** Output side of the connection. */
	UPROPERTY()
	FScriptableGraphPinRef From;

	/** Input side of the connection. */
	UPROPERTY()
	FScriptableGraphPinRef To;

	bool operator==(const FScriptableGraphConnection& Other) const
	{
		return From == Other.From && To == Other.To;
	}
};
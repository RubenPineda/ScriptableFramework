// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphSchema.h"
#include "AssetRegistry/AssetData.h"

/** Schema action used as the payload for palette drags. */
struct FScriptablePaletteAction : public FEdGraphSchemaAction
{
	static FName StaticGetTypeId() { static FName Type("FScriptablePaletteAction"); return Type; }
	virtual FName GetTypeId() const override { return StaticGetTypeId(); }

	const UStruct* StructPayload = nullptr;
	FAssetData AssetPayload;

	FScriptablePaletteAction() = default;
	FScriptablePaletteAction(FText InCategory, FText InMenuDesc, FText InTooltip, int32 InGrouping, const UStruct* InStruct, const FAssetData& InAsset)
		: FEdGraphSchemaAction(MoveTemp(InCategory), MoveTemp(InMenuDesc), MoveTemp(InTooltip), InGrouping)
		, StructPayload(InStruct)
		, AssetPayload(InAsset)
	{
	}

	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, TArray<UEdGraphPin*>& FromPins, const FVector2f& Location, bool bSelectNewNode = true) override;
};
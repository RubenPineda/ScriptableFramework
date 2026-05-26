// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "KismetPins/SGraphPinExec.h"

/**
 * Custom pin widget for scriptable graph pins. Renders the standard BP-style exec triangle
 * (hollow when disconnected, filled when connected, routed behind the node) and respects
 * the owning ed-node's per-pin label visibility hint.
 */
class SCRIPTABLEFRAMEWORKEDITOR_API SScriptableGraphPin : public SGraphPinExec
{
public:
	SLATE_BEGIN_ARGS(SScriptableGraphPin) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InPin);

protected:
	//~ SGraphPin interface
	virtual TSharedRef<SWidget> GetDefaultValueWidget() override;
	virtual FSlateColor GetPinColor() const override { return FLinearColor::White; }
	//~ End of SGraphPin interface

	/** Resolves the label visibility by asking the owning UScriptableEdGraphNode. */
	EVisibility OnGetLabelVisibility() const;
};
// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "SGraphNode.h"

class UScriptableEdGraphNode_Task;

/**
 * Slate widget for UScriptableEdGraphNode_Task. Adds a top-right "xN" / "x∞" overlay badge when the
 * wrapped task has Loop enabled, mirroring the Loop pill shown in the details panel.
 */
class SScriptableGraphNode_Task : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SScriptableGraphNode_Task) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UScriptableEdGraphNode_Task* InNode);

	//~ SGraphNode interface
	virtual TArray<FOverlayWidgetInfo> GetOverlayWidgets(bool bSelected, const FVector2f& WidgetSize) const override;
	//~ End of SGraphNode interface

private:
	/** Builds a rounded badge (label + tint) for an FOverlayWidgetInfo. Centralizes badge padding/font/radius so all node badges match. */
	TSharedRef<SWidget> MakeBadge(const FText& Label, const FLinearColor& TintColor) const;

	/** Weak ref to the editor node, re-resolved each GetOverlayWidgets so the badge survives task swaps / ReconstructNode. */
	TWeakObjectPtr<UScriptableEdGraphNode_Task> EdTaskNode;
};
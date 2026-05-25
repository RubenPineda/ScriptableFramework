// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "SGraphNode.h"

class UScriptableEdGraphNode_Task;

/**
 * Slate widget for UScriptableEdGraphNode_Task. Inherits the default node visuals from SGraphNode
 * and contributes a single overlay widget: a small "xN" / "x∞" badge anchored to the top-right
 * corner, visible only when the wrapped task has Loop enabled. The badge mirrors the Loop pill
 * shown in the details panel so the same setting is recognizable in both surfaces.
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
	/** Builds a small rounded badge with the given label and tint, ready to be plugged into an FOverlayWidgetInfo. Centralizes the visual contract for all node badges (Loop, DoOnce, future flow markers) so they share padding, font and corner radius. */
	TSharedRef<SWidget> MakeBadge(const FText& Label, const FLinearColor& TintColor) const;

	/** Weak ref back to the editor node. Resolved each call to GetOverlayWidgets so the badge stays correct across task swaps and ReconstructNode calls. */
	TWeakObjectPtr<UScriptableEdGraphNode_Task> EdTaskNode;
};
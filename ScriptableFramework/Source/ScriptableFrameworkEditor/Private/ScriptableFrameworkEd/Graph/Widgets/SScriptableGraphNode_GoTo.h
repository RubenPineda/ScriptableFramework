// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "SGraphNode.h"

class UScriptableEdGraphNode;
class UScriptableNode_GoTo;

/**
 * Slate widget for UScriptableEdGraphNode_GoTo. Adds an on-node dropdown listing every ReceiveEvent
 * name in the graph; picking one sets the node's TargetEvent and updates its "Go to X" title.
 */
class SScriptableGraphNode_GoTo : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SScriptableGraphNode_GoTo) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UScriptableEdGraphNode* InNode);

	//~ SGraphNode interface
	virtual void CreateBelowPinControls(TSharedPtr<SVerticalBox> MainBox) override;
	//~ End of SGraphNode interface

private:
	UScriptableNode_GoTo* GetRuntimeGoTo() const;

	/** Label for the dropdown button: the current TargetEvent, or a placeholder. */
	FText GetSelectedEventText() const;

	/** Builds the menu of available ReceiveEvent names in the owning graph. */
	TSharedRef<SWidget> BuildEventMenu();

	/** Assigns the picked event to the runtime node and refreshes the title. */
	void OnEventPicked(FName EventName);
};

// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "SGraphNode.h"

class UScriptableEdGraphNode;
class UScriptableNode_AND;

/**
 * Slate widget for UScriptableEdGraphNode_AND. Adds the FlowGraph / K2-style "Add pin" affordance
 * to the *input* side of the node so the user can grow the gate's input set directly from the
 * canvas. Mirrors SScriptableGraphNode_Sequence but on the input column.
 */
class SScriptableGraphNode_AND : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SScriptableGraphNode_AND) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UScriptableEdGraphNode* InNode);

	//~ SGraphNode interface
	virtual void CreateInputSideAddButton(TSharedPtr<SVerticalBox> InputBox) override;
	virtual FReply OnAddPin() override;
	virtual EVisibility IsAddPinButtonVisible() const override;
	//~ End of SGraphNode interface

private:
	UScriptableNode_AND* GetRuntimeAND() const;
};
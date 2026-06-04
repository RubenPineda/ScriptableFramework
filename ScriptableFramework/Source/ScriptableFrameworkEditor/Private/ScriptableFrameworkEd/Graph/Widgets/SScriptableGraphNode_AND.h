// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableFrameworkEd/Graph/Widgets/SScriptableGraphNode_Base.h"

class UScriptableEdGraphNode;
class UScriptableNode_AND;

/**
 * Slate widget for UScriptableEdGraphNode_AND. Adds the K2-style "Add pin" affordance on the input
 * side so the user can grow the gate's inputs from the canvas. Mirrors SScriptableGraphNode_Sequence.
 */
class SScriptableGraphNode_AND : public SScriptableGraphNode_Base
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
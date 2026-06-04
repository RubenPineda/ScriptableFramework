// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableFrameworkEd/Graph/Widgets/SScriptableGraphNode_Base.h"

class UScriptableEdGraphNode;
class UScriptableNode_OR;

/**
 * Slate widget for UScriptableEdGraphNode_OR. Adds the K2-style "Add pin" affordance on the input
 * side. Mirrors SScriptableGraphNode_AND.
 */
class SScriptableGraphNode_OR : public SScriptableGraphNode_Base
{
public:
	SLATE_BEGIN_ARGS(SScriptableGraphNode_OR) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UScriptableEdGraphNode* InNode);

	//~ SGraphNode interface
	virtual void CreateInputSideAddButton(TSharedPtr<SVerticalBox> InputBox) override;
	virtual FReply OnAddPin() override;
	virtual EVisibility IsAddPinButtonVisible() const override;
	//~ End of SGraphNode interface

private:
	UScriptableNode_OR* GetRuntimeOR() const;
};

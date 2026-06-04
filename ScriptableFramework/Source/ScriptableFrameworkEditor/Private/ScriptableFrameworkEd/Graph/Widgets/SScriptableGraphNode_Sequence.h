// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableFrameworkEd/Graph/Widgets/SScriptableGraphNode_Base.h"

class UScriptableEdGraphNode;
class UScriptableNode_Sequence;

/** Slate widget for graph nodes wrapping a UScriptableNode_Sequence. */
class SScriptableGraphNode_Sequence : public SScriptableGraphNode_Base
{
public:
	SLATE_BEGIN_ARGS(SScriptableGraphNode_Sequence) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UScriptableEdGraphNode* InNode);

	//~ SGraphNode interface
	virtual void CreateOutputSideAddButton(TSharedPtr<SVerticalBox> OutputBox) override;
	virtual FReply OnAddPin() override;
	virtual EVisibility IsAddPinButtonVisible() const override;
	//~ End of SGraphNode interface

private:
	/** Resolves the runtime Sequence node from SGraphNode::GraphNode. Null only during teardown. */
	UScriptableNode_Sequence* GetRuntimeSequence() const;
};
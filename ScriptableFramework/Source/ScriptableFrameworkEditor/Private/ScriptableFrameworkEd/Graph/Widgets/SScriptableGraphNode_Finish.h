// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "SGraphNode.h"

class UScriptableEdGraphNode;
class UScriptableNode_Finish;

/** On-node dropdown listing every available output (graph's Outputs + Exit built-ins). Mirrors GoTo. */
class SScriptableGraphNode_Finish : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SScriptableGraphNode_Finish) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UScriptableEdGraphNode* InNode);

	//~ SGraphNode interface
	virtual void CreateBelowPinControls(TSharedPtr<SVerticalBox> MainBox) override;
	//~ End of SGraphNode interface

private:
	UScriptableNode_Finish* GetRuntimeFinish() const;

	FText GetSelectedOutputText() const;
	TSharedRef<SWidget> BuildOutputMenu();
	void OnOutputPicked(FName OutputName);

	/** Hidden when the parent graph has no user-declared Outputs. Polled each paint so adding the
	 * first Output in the asset shows the dropdown without rebuilding the slate node. */
	EVisibility GetComboVisibility() const;
};

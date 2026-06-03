// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class IPropertyHandle;
class UScriptableNode_Finish;
class UScriptableGraph;

/**
 * Replaces UScriptableNode_Finish::OutputName with a SComboButton listing the parent graph's
 * declared Outputs. Mirrors the on-node dropdown so both surfaces stay in sync. Hides the row
 * entirely when Graph.Outputs is empty (matches the on-node behaviour).
 */
class FScriptableNodeFinishCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance() { return MakeShared<FScriptableNodeFinishCustomization>(); }

	//~ IDetailCustomization interface
	virtual void CustomizeDetails(class IDetailLayoutBuilder& DetailBuilder) override;
	//~ End of IDetailCustomization interface

private:
	FText GetSelectedOutputText() const;
	TSharedRef<SWidget> BuildOutputMenu();
	void OnOutputPicked(FName OutputName);

	TSharedPtr<IPropertyHandle> OutputNameHandle;
	TWeakObjectPtr<UScriptableNode_Finish> WeakFinish;
	TWeakObjectPtr<UScriptableGraph> WeakGraph;
};

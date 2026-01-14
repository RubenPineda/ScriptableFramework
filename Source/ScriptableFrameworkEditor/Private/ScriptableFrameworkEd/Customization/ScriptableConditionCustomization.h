// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableObjectCustomization.h"

class IPropertyHandle;

class FScriptableConditionCustomization : public FScriptableObjectCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance() { return MakeShareable(new FScriptableConditionCustomization); }

protected:
	virtual UClass* GetWrapperClass() const override;
	virtual TSharedPtr<SHorizontalBox> GetHeaderValueContent() override;

private:
	// --- Negate Logic ---
	bool GetNegateValue() const;
	FReply OnNegateClicked();
	FSlateColor GetNegateColor() const;
	FText GetNegateText() const;
	FText GetNegateTooltip() const;
};
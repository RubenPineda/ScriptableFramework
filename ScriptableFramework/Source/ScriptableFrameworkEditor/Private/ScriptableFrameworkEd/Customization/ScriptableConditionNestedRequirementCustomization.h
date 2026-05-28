// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableConditionCustomization.h"

class SComboButton;

/**
 * Customization for UScriptableCondition_NestedRequirement. Merges the wrapper condition's header
 * (checkbox, mode toggle, rename field) with the inner FScriptableRequirement's UI, and hides the
 * inner Context button so the nested unit inherits the parent scope instead of declaring its own.
 */
class FScriptableConditionNestedRequirementCustomization : public FScriptableConditionCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance() { return MakeShareable(new FScriptableConditionNestedRequirementCustomization); }

	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	virtual void InitCustomization(TSharedRef<IPropertyHandle> InPropertyHandle, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual TSharedPtr<SHorizontalBox> GetHeaderNameContent() override;
	virtual TSharedPtr<SHorizontalBox> GetHeaderExtensionContent() override;

private:
	/** Helper instance to handle the inner requirement logic */
	TSharedPtr<class FScriptableRequirementCustomization> RequirementCustomization;

	FText GetRequirementName() const;
	void OnRequirementNameCommitted(const FText& NewText, ETextCommit::Type CommitInfo);
	bool VerifyRequirementNameChanged(const FText& NewText, FText& OutErrorMessage);
};

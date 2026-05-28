// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableTaskCustomization.h"

/**
 * Customization for UScriptableTask_NestedAction. Merges the wrapper task's header (checkbox,
 * Loop/DoOnce badges, rename field) with the inner FScriptableAction's UI, and hides the inner
 * Context button so the nested unit inherits the parent scope.
 */
class FScriptableTaskNestedActionCustomization : public FScriptableTaskCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance() { return MakeShareable(new FScriptableTaskNestedActionCustomization); }

	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	virtual void InitCustomization(TSharedRef<IPropertyHandle> InPropertyHandle, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual TSharedPtr<SHorizontalBox> GetHeaderNameContent() override;
	virtual TSharedPtr<SHorizontalBox> GetHeaderExtensionContent() override;

private:
	/** Helper instance to handle the inner action logic */
	TSharedPtr<class FScriptableActionCustomization> ActionCustomization;

	FText GetActionName() const;
	void OnActionNameCommitted(const FText& NewText, ETextCommit::Type CommitInfo);
	bool VerifyActionNameChanged(const FText& NewText, FText& OutErrorMessage);
};

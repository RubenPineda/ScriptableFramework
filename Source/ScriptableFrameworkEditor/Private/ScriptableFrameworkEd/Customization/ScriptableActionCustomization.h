// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"

class IPropertyUtilities;

class FScriptableActionCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
	TSharedPtr<IPropertyHandle> ActionHandle;
	TSharedPtr<IPropertyHandle> TasksHandle;

	// We keep this as SComboButton pointer to avoid forcing include of SScriptableTypePicker in header
	TSharedPtr<class SComboButton> AddTaskComboButton;

	TSharedPtr<IPropertyUtilities> PropertyUtilities;

	/** Callback when a class is selected from the picker. */
	void OnTaskClassPicked(const UStruct* InStruct, const struct FAssetData& AssetData);

	/** Helper to actually instantiate the task into the array. */
	TSharedPtr<IPropertyHandle> AddTask(const UClass* TaskClass);

	/** Custom row generator to make tasks look like StateTree nodes (Clean header, Icon, Buttons). */
	void OnGenerateElement(TSharedRef<IPropertyHandle> ElementHandle, int32 Index, IDetailChildrenBuilder& Builder);
};
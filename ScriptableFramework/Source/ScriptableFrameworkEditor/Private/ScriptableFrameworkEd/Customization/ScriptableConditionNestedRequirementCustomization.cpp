// Copyright 2026 kirzo

#include "ScriptableConditionNestedRequirementCustomization.h"
#include "ScriptableRequirementCustomization.h"

#include "ScriptableConditions/ScriptableCondition_NestedRequirement.h"
#include "ScriptableConditions/ScriptableCondition.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailPropertyRow.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"

#define LOCTEXT_NAMESPACE "FScriptableConditionNestedRequirementCustomization"

void FScriptableConditionNestedRequirementCustomization::InitCustomization(TSharedRef<IPropertyHandle> InPropertyHandle, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	FScriptableConditionCustomization::InitCustomization(InPropertyHandle, CustomizationUtils);

	RequirementCustomization = MakeShared<FScriptableRequirementCustomization>();

	// The nested unit inherits the parent scope's context; hide the inner Context button.
	RequirementCustomization->bShowContextButton = false;

	TSharedPtr<IPropertyHandle> ReqHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(UScriptableCondition_NestedRequirement, Requirement));
	if (ReqHandle.IsValid())
	{
		RequirementCustomization->InitCustomization(ReqHandle.ToSharedRef(), CustomizationUtils);
	}
}

TSharedPtr<SHorizontalBox> FScriptableConditionNestedRequirementCustomization::GetHeaderNameContent()
{
	TSharedPtr<SHorizontalBox> NameBox = FScriptableConditionCustomization::GetHeaderNameContent();
	// Remove default name
	if (NameBox.IsValid() && NameBox->IsValidSlotIndex(1))
	{
		NameBox->RemoveSlot(NameBox->GetSlot(1).GetWidget());
	}

	NameBox->AddSlot()
		.FillWidth(1.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SInlineEditableTextBlock)
				.Text(this, &FScriptableConditionNestedRequirementCustomization::GetRequirementName)
				.OnTextCommitted(this, &FScriptableConditionNestedRequirementCustomization::OnRequirementNameCommitted)
				.OnVerifyTextChanged(this, &FScriptableConditionNestedRequirementCustomization::VerifyRequirementNameChanged)
				.Font(IDetailLayoutBuilder::GetDetailFontBold())
				.ToolTipText(LOCTEXT("RenameTooltip", "Click to rename this nested requirement."))
				.ColorAndOpacity(ScriptableObject.IsValid() ? FSlateColor::UseForeground() : FSlateColor::UseSubduedForeground())
		];

	return NameBox;
}

TSharedPtr<SHorizontalBox> FScriptableConditionNestedRequirementCustomization::GetHeaderExtensionContent()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			RequirementCustomization->GetHeaderExtensionContent().ToSharedRef()
		]

		// Spacer
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(8, 0, 0, 0)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			FScriptableConditionCustomization::GetHeaderExtensionContent().ToSharedRef()
		];
}

void FScriptableConditionNestedRequirementCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	if (RequirementCustomization.IsValid())
	{
		TSharedPtr<IPropertyHandle> ReqHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(UScriptableCondition_NestedRequirement, Requirement));
		if (ReqHandle.IsValid())
		{
			RequirementCustomization->CustomizeChildren(ReqHandle.ToSharedRef(), ChildBuilder, CustomizationUtils);
		}
	}
}

FText FScriptableConditionNestedRequirementCustomization::GetRequirementName() const
{
	if (PropertyHandle.IsValid())
	{
		TSharedPtr<IPropertyHandle> NameHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(UScriptableCondition_NestedRequirement, RequirementName));
		if (NameHandle.IsValid())
		{
			FString Val;
			NameHandle->GetValue(Val);

			if (!Val.IsEmpty())
			{
				return FText::FromString(Val);
			}
		}
	}

	return LOCTEXT("DefaultRequirementName", "Nested Requirement");
}

void FScriptableConditionNestedRequirementCustomization::OnRequirementNameCommitted(const FText& NewText, ETextCommit::Type CommitInfo)
{
	if (PropertyHandle.IsValid())
	{
		TSharedPtr<IPropertyHandle> NameHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(UScriptableCondition_NestedRequirement, RequirementName));
		if (NameHandle.IsValid())
		{
			FString CurrentVal;
			NameHandle->GetValue(CurrentVal);

			FString NewString = NewText.ToString();

			if (NewString.Equals(LOCTEXT("DefaultRequirementName", "Nested Requirement").ToString(), ESearchCase::IgnoreCase))
			{
				NewString.Empty();
			}

			if (!CurrentVal.Equals(NewString))
			{
				FScopedTransaction Transaction(LOCTEXT("RenameRequirement", "Rename Nested Requirement"));
				NameHandle->SetValue(NewString);
			}
		}
	}
}

bool FScriptableConditionNestedRequirementCustomization::VerifyRequirementNameChanged(const FText& NewText, FText& OutErrorMessage)
{
	return true;
}

#undef LOCTEXT_NAMESPACE

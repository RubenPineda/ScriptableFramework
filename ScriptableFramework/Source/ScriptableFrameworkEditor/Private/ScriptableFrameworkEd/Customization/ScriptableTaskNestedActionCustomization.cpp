// Copyright 2026 kirzo

#include "ScriptableTaskNestedActionCustomization.h"
#include "ScriptableActionCustomization.h"

#include "ScriptableTasks/ScriptableTask_NestedAction.h"
#include "ScriptableTasks/ScriptableTask.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailPropertyRow.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"

#define LOCTEXT_NAMESPACE "FScriptableTaskNestedActionCustomization"

void FScriptableTaskNestedActionCustomization::InitCustomization(TSharedRef<IPropertyHandle> InPropertyHandle, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	FScriptableTaskCustomization::InitCustomization(InPropertyHandle, CustomizationUtils);

	ActionCustomization = MakeShared<FScriptableActionCustomization>();

	// The nested unit inherits the parent scope's context; hide the inner Context button.
	ActionCustomization->bShowContextButton = false;

	TSharedPtr<IPropertyHandle> ActionHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(UScriptableTask_NestedAction, Action));
	if (ActionHandle.IsValid())
	{
		ActionCustomization->InitCustomization(ActionHandle.ToSharedRef(), CustomizationUtils);
	}
}

TSharedPtr<SHorizontalBox> FScriptableTaskNestedActionCustomization::GetHeaderNameContent()
{
	TSharedPtr<SHorizontalBox> NameBox = FScriptableTaskCustomization::GetHeaderNameContent();
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
				.Text(this, &FScriptableTaskNestedActionCustomization::GetActionName)
				.OnTextCommitted(this, &FScriptableTaskNestedActionCustomization::OnActionNameCommitted)
				.OnVerifyTextChanged(this, &FScriptableTaskNestedActionCustomization::VerifyActionNameChanged)
				.Font(IDetailLayoutBuilder::GetDetailFontBold())
				.ToolTipText(LOCTEXT("RenameTooltip", "Click to rename this nested action."))
				.ColorAndOpacity(ScriptableObject.IsValid() ? FSlateColor::UseForeground() : FSlateColor::UseSubduedForeground())
		];

	return NameBox;
}

TSharedPtr<SHorizontalBox> FScriptableTaskNestedActionCustomization::GetHeaderExtensionContent()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			ActionCustomization->GetHeaderExtensionContent().ToSharedRef()
		]

		// Spacer
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(8, 0, 0, 0)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			FScriptableTaskCustomization::GetHeaderExtensionContent().ToSharedRef()
		];
}

void FScriptableTaskNestedActionCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	if (ActionCustomization.IsValid())
	{
		TSharedPtr<IPropertyHandle> ActionHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(UScriptableTask_NestedAction, Action));
		if (ActionHandle.IsValid())
		{
			ActionCustomization->CustomizeChildren(ActionHandle.ToSharedRef(), ChildBuilder, CustomizationUtils);
		}
	}
}

FText FScriptableTaskNestedActionCustomization::GetActionName() const
{
	if (PropertyHandle.IsValid())
	{
		TSharedPtr<IPropertyHandle> NameHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(UScriptableTask_NestedAction, ActionName));
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

	return LOCTEXT("DefaultActionName", "Nested Action");
}

void FScriptableTaskNestedActionCustomization::OnActionNameCommitted(const FText& NewText, ETextCommit::Type CommitInfo)
{
	if (PropertyHandle.IsValid())
	{
		TSharedPtr<IPropertyHandle> NameHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(UScriptableTask_NestedAction, ActionName));
		if (NameHandle.IsValid())
		{
			FString CurrentVal;
			NameHandle->GetValue(CurrentVal);

			FString NewString = NewText.ToString();

			if (NewString.Equals(LOCTEXT("DefaultActionName", "Nested Action").ToString(), ESearchCase::IgnoreCase))
			{
				NewString.Empty();
			}

			if (!CurrentVal.Equals(NewString))
			{
				FScopedTransaction Transaction(LOCTEXT("RenameAction", "Rename Nested Action"));
				NameHandle->SetValue(NewString);
			}
		}
	}
}

bool FScriptableTaskNestedActionCustomization::VerifyActionNameChanged(const FText& NewText, FText& OutErrorMessage)
{
	return true;
}

#undef LOCTEXT_NAMESPACE

#include "ScriptableObjectCustomization.h"
#include "ScriptableObject.h"

#include "ScriptableFrameworkEditorHelpers.h"

#include "PropertyCustomizationHelpers.h"

#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailPropertyRow.h"
#include "UObject/Field.h"
#include "IPropertyUtilities.h"

UE_DISABLE_OPTIMIZATION

#define LOCTEXT_NAMESPACE "FScriptableObjectCustomization"

void FScriptableObjectCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	TArray<UObject*> OuterObjects;
	InPropertyHandle->GetOuterObjects(OuterObjects);

	if (OuterObjects.Num() > 1)
	{
		return;
	}

	PropertyUtilities = CustomizationUtils.GetPropertyUtilities();
	PropertyHandle = InPropertyHandle;

	TSharedPtr<IPropertyHandle> ChildPropertyHandle = PropertyHandle->GetChildHandle(0);
	if (ChildPropertyHandle)
	{
		// Get the actual ScriptableObject
		TArray<void*> RawData;
		ChildPropertyHandle->AccessRawData(RawData);
		ScriptableObject = static_cast<UScriptableObject*>(RawData[0]);

		bIsBlueprintClass = IsValid(ScriptableObject) && ScriptableObject->GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint);

		EnabledPropertyHandle = ChildPropertyHandle->GetChildHandle("bEnabled");
		GatherChildProperties(ChildPropertyHandle);
	}

	HorizontalBox = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SCheckBox)
				.ToolTipText(LOCTEXT("ScriptableObjectEnabledTooltip", "Enable or disable the object."))
				.ForegroundColor(FColor::Green)
				.IsChecked(this, &FScriptableObjectCustomization::GetEnabledCheckBoxState)
				.OnCheckStateChanged(this, &FScriptableObjectCustomization::OnEnabledCheckBoxChanged)
		]
		+ SHorizontalBox::Slot()
		.FillWidth(0.5f)
		[
			PropertyHandle->CreatePropertyValueWidget(false)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			PropertyCustomizationHelpers::MakeUseSelectedButton(FSimpleDelegate::CreateSP(this, &FScriptableObjectCustomization::OnUseSelected))
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			PropertyCustomizationHelpers::MakeClearButton(FSimpleDelegate::CreateSP(this, &FScriptableObjectCustomization::OnClear))
		];

	if (bIsBlueprintClass)
	{
		const FText Text = FText::Format(FText::FromString("Browse to '{0}' in Content Browser"), ScriptableObject->GetClass()->GetDisplayNameText());
		HorizontalBox->InsertSlot(3)
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				PropertyCustomizationHelpers::MakeBrowseButton(FSimpleDelegate::CreateSP(this, &FScriptableObjectCustomization::OnBrowseTo), Text)
			];
	}

	HeaderRow
		.NameContent()
		[
			PropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			HorizontalBox.ToSharedRef()
		];
}

void FScriptableObjectCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	PropertyUtilities = CustomizationUtils.GetPropertyUtilities();

	uint32 NumberOfChild;
	if (InPropertyHandle->GetNumChildren(NumberOfChild) == FPropertyAccess::Success)
	{
		for (uint32 Index = 0; Index < NumberOfChild; ++Index)
		{
			TSharedRef<IPropertyHandle> CategoryPropertyHandle = InPropertyHandle->GetChildHandle(Index).ToSharedRef();

			// Don't add category rows. Only iterate over the category
			uint32 NumberOfChildrenInCategory;
			CategoryPropertyHandle->GetNumChildren(NumberOfChildrenInCategory);
			for (uint32 ChildrenInCategoryIndex = 0; ChildrenInCategoryIndex < NumberOfChildrenInCategory; ++ChildrenInCategoryIndex)
			{
				TSharedRef<IPropertyHandle> SubPropertyHandle = CategoryPropertyHandle->GetChildHandle(ChildrenInCategoryIndex).ToSharedRef();

				if (ScriptableFrameworkEditor::IsPropertyVisible(SubPropertyHandle))
				{
					ChildBuilder.AddProperty(SubPropertyHandle);
				}
			}
		}
	}
}

ECheckBoxState FScriptableObjectCustomization::GetEnabledCheckBoxState() const
{
	bool bEnabled = false;

	if (EnabledPropertyHandle.IsValid())
	{
		EnabledPropertyHandle->GetValue(bEnabled);
	}

	return bEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void FScriptableObjectCustomization::OnEnabledCheckBoxChanged(ECheckBoxState NewCheckedState)
{
	const bool bEnabled = (NewCheckedState == ECheckBoxState::Checked);
	if (EnabledPropertyHandle.IsValid())
	{
		EnabledPropertyHandle->SetValue(bEnabled);
		EnabledPropertyHandle->RequestRebuildChildren();
	}
}

void FScriptableObjectCustomization::SetScriptableObjectClass(TSubclassOf<UObject> ScriptableObjectClass)
{
	UClass* MyClass = nullptr;
	if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(PropertyHandle->GetProperty()))
	{
		MyClass = ObjectProperty->PropertyClass;
	}

	if (ScriptableObjectClass->IsChildOf(MyClass))
	{
		const FScopedTransaction Transaction(FText::FromString("Set Scriptable Object"));

		TArray<UObject*> OuterObjects;
		PropertyHandle->GetOuterObjects(OuterObjects);

		OuterObjects[0]->Modify();
		PropertyCustomizationHelpers::CreateNewInstanceOfEditInlineObjectClass(PropertyHandle.ToSharedRef(), ScriptableObjectClass, EPropertyValueSetFlags::InteractiveChange);
		PropertyUtilities->NotifyFinishedChangingProperties(FPropertyChangedEvent(PropertyHandle->GetProperty()));
		// Extra end transaction because we use 'Interactive Change' in the CreateNewInstance call
		GEditor->EndTransaction();
		PropertyUtilities->ForceRefresh();
		return;
	}
}

void FScriptableObjectCustomization::OnUseSelected()
{
	UClass* MyClass = nullptr;
	if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(PropertyHandle->GetProperty()))
	{
		MyClass = ObjectProperty->PropertyClass;
	}

	TArray<FAssetData> SelectedAssets;
	GEditor->GetContentBrowserSelections(SelectedAssets);

	for (const FAssetData& AssetData : SelectedAssets)
	{
		UBlueprint* SelectedBlueprint = Cast<UBlueprint>(AssetData.GetAsset());

		if (SelectedBlueprint)
		{
			if (SelectedBlueprint->GeneratedClass && SelectedBlueprint->GeneratedClass->IsChildOf(MyClass))
			{
				SetScriptableObjectClass(SelectedBlueprint->GeneratedClass);
				return;
			}
		}
	}
}

void FScriptableObjectCustomization::OnBrowseTo()
{
	if (ScriptableObject)
	{
		TArray<FAssetData> SyncAssets;
		SyncAssets.Add(FAssetData(ScriptableObject->GetClass()));
		GEditor->SyncBrowserToObjects(SyncAssets);
	}
}

void FScriptableObjectCustomization::OnClear()
{
	static const FString None("None");
	PropertyHandle->SetValueFromFormattedString(None);
}

#undef LOCTEXT_NAMESPACE

UE_ENABLE_OPTIMIZATION
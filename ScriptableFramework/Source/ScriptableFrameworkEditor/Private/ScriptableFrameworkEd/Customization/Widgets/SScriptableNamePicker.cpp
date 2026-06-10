// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Customization/Widgets/SScriptableNamePicker.h"

#include "ScriptablePropertyUtilities.h"
#include "Core/KzNamedVariant.h"
#include "KzPinTypeUtils.h"

#include "EdGraphSchema_K2.h"
#include "Kismet2/BlueprintEditorUtils.h"

#include "DetailLayoutBuilder.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBox.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "Textures/SlateIcon.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

TArray<FScriptableNameItem> ScriptableNamePickerItems::FromLocals(const UObject* Node, TFunctionRef<bool(const FKzNamedVariant&)> Filter)
{
	TArray<FScriptableNameItem> OutItems;
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	for (const FKzNamedVariant& Var : FScriptablePropertyUtilities::FindLocalsDeclarationFor(Node))
	{
		if (Var.GetName().IsNone() || !Filter(Var)) continue;

		const FEdGraphPinType PinType = KzLib::Editor::PinTypeFromBagType(Var.GetValue().GetType(), Var.GetValue().GetTypeObject(), EPropertyBagContainerType::None);

		FScriptableNameItem& Item = OutItems.AddDefaulted_GetRef();
		Item.Name = Var.GetName();
		Item.Icon = FBlueprintEditorUtils::GetIconFromPin(PinType, true);
		Item.IconColor = Schema->GetPinTypeColor(PinType);
		Item.ToolTip = UEdGraphSchema_K2::TypeToText(PinType);
	}

	OutItems.Sort([](const FScriptableNameItem& A, const FScriptableNameItem& B) { return A.Name.LexicalLess(B.Name); });
	return OutItems;
}

TArray<FScriptableNameItem> ScriptableNamePickerItems::FromFunctions(const UClass* Class, TFunctionRef<bool(const UFunction*)> Filter)
{
	TArray<FScriptableNameItem> OutItems;
	if (!Class) return OutItems;

	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	// TFieldIterator walks the most-derived class first, so an override shadows its super version.
	TSet<FName> Seen;
	for (TFieldIterator<UFunction> It(Class); It; ++It)
	{
		const UFunction* Func = *It;
		if (Seen.Contains(Func->GetFName()) || !Filter(Func)) continue;
		Seen.Add(Func->GetFName());

		FScriptableNameItem& Item = OutItems.AddDefaulted_GetRef();
		Item.Name = Func->GetFName();
		Item.Icon = FAppStyle::GetBrush("GraphEditor.Function_16x");
		Item.ToolTip = Func->GetToolTipText();

		if (const FProperty* ReturnProp = Func->GetReturnProperty())
		{
			FEdGraphPinType PinType;
			Schema->ConvertPropertyToPinType(ReturnProp, PinType);
			Item.IconColor = Schema->GetPinTypeColor(PinType);
		}
	}

	OutItems.Sort([](const FScriptableNameItem& A, const FScriptableNameItem& B) { return A.Name.LexicalLess(B.Name); });
	return OutItems;
}

void SScriptableNamePicker::Construct(const FArguments& InArgs)
{
	SectionTitle = InArgs._SectionTitle;
	CurrentName = InArgs._CurrentName;
	OnGetItems = InArgs._OnGetItems;
	OnNamePicked = InArgs._OnNamePicked;

	RefreshItems();

	// Standard details-row combo look: the "ComboBox" style (what SComboBox-based property combos
	// use) with its own content padding, plus a 16px-high icon box, keeps the native button height.
	const FComboBoxStyle& ComboStyle = FAppStyle::Get().GetWidgetStyle<FComboBoxStyle>("ComboBox");
	SComboButton::Construct(SComboButton::FArguments()
		.ComboButtonStyle(&ComboStyle.ComboButtonStyle)
		.ContentPadding(ComboStyle.ContentPadding)
		.OnGetMenuContent(this, &SScriptableNamePicker::BuildMenuContent)
		.ButtonContent()
		[
			SNew(SHorizontalBox)
				.Clipping(EWidgetClipping::ClipToBounds)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SBox)
						.HeightOverride(16.0f)
						.Visibility(this, &SScriptableNamePicker::GetCurrentIconVisibility)
						[
							SNew(SImage)
								.Image(this, &SScriptableNamePicker::GetCurrentIcon)
								.ColorAndOpacity(this, &SScriptableNamePicker::GetCurrentIconColor)
						]
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				.Padding(4.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
						.Font(IDetailLayoutBuilder::GetDetailFont())
						.Text(this, &SScriptableNamePicker::GetCurrentText)
				]
		]);
}

TSharedRef<SWidget> SScriptableNamePicker::BuildMenuContent()
{
	RefreshItems();

	// Mirrors SPropertyBinding::OnGenerateDelegateMenu so the menu matches the binding one exactly:
	// searchable multibox menu, section headings, and spacer+icon+name entry widgets.
	const bool bShouldCloseWindowAfterMenuSelection = true;
	const bool bSearchableMenu = true;
	const bool bRecursivelySearchableMenu = false;
	FMenuBuilder MenuBuilder(bShouldCloseWindowAfterMenuSelection, nullptr, nullptr, /*bCloseSelfOnly*/false, &FCoreStyle::Get(), bSearchableMenu, NAME_None, bRecursivelySearchableMenu);

	MenuBuilder.BeginSection(NAME_None, SectionTitle);

	if (!CurrentName.Get(NAME_None).IsNone())
	{
		MenuBuilder.AddMenuEntry(
			INVTEXT("Clear"),
			INVTEXT("Clears the current selection."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Cross"),
			FUIAction(FExecuteAction::CreateSP(this, &SScriptableNamePicker::HandleNamePicked, FName(NAME_None))));
	}

	for (const FScriptableNameItem& Item : Items)
	{
		MenuBuilder.AddMenuEntry(
			FUIAction(FExecuteAction::CreateSP(this, &SScriptableNamePicker::HandleNamePicked, Item.Name)),
			SNew(SHorizontalBox)
				.ToolTipText(Item.ToolTip)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SSpacer)
						.Size(FVector2D(18.0f, 0.0f))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(1.0f, 0.0f)
				[
					SNew(SImage)
						.Image(Item.Icon)
						.ColorAndOpacity(Item.IconColor)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(4.0f, 0.0f)
				[
					SNew(STextBlock)
						.Text(FText::FromName(Item.Name))
				]);
	}
	MenuBuilder.EndSection();

	FDisplayMetrics DisplayMetrics;
	FSlateApplication::Get().GetCachedDisplayMetrics(DisplayMetrics);

	// The multibox hides its search field while the menu has fewer entries than the global
	// MenuSearchFieldVisibilityThreshold; our pickers always want it. Force the wrapping block
	// visible (overrides its bound visibility) and focus the field so the user can type right away.
	const TSharedRef<SWidget> MenuWidget = MenuBuilder.MakeWidget();
	const TSharedRef<SMultiBoxWidget> MultiBoxWidget = StaticCastSharedRef<SMultiBoxWidget>(MenuWidget);
	if (TSharedPtr<SWidget> SearchWidget = MultiBoxWidget->GetSearchTextWidget())
	{
		if (TSharedPtr<SWidget> SearchContainer = SearchWidget->GetParentWidget())
		{
			SearchContainer->SetVisibility(EVisibility::Visible);
		}
		SetMenuContentWidgetToFocus(SearchWidget);
	}

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.MaxHeight(DisplayMetrics.PrimaryDisplayHeight * 0.5)
		[
			MenuWidget
		];
}

void SScriptableNamePicker::HandleNamePicked(FName PickedName)
{
	OnNamePicked.ExecuteIfBound(PickedName);
}

void SScriptableNamePicker::RefreshItems()
{
	Items.Reset();
	if (OnGetItems.IsBound())
	{
		Items = OnGetItems.Execute();
	}
}

const FScriptableNameItem* SScriptableNamePicker::FindCurrentItem() const
{
	const FName Name = CurrentName.Get(NAME_None);
	if (Name.IsNone()) return nullptr;

	return Items.FindByPredicate([&Name](const FScriptableNameItem& Item) { return Item.Name == Name; });
}

const FSlateBrush* SScriptableNamePicker::GetCurrentIcon() const
{
	const FScriptableNameItem* Item = FindCurrentItem();
	return Item ? Item->Icon : nullptr;
}

FSlateColor SScriptableNamePicker::GetCurrentIconColor() const
{
	const FScriptableNameItem* Item = FindCurrentItem();
	return Item ? Item->IconColor : FSlateColor::UseForeground();
}

EVisibility SScriptableNamePicker::GetCurrentIconVisibility() const
{
	return GetCurrentIcon() ? EVisibility::Visible : EVisibility::Collapsed;
}

FText SScriptableNamePicker::GetCurrentText() const
{
	const FName Name = CurrentName.Get(NAME_None);
	return Name.IsNone() ? INVTEXT("None") : FText::FromName(Name);
}
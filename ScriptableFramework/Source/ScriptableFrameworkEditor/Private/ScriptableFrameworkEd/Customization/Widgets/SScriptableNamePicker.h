// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateColor.h"
#include "Widgets/Input/SComboButton.h"

struct FKzNamedVariant;
struct FSlateBrush;

/** Entry shown by SScriptableNamePicker: a name plus the type glyph rendered next to it. */
struct FScriptableNameItem
{
	FName Name;
	const FSlateBrush* Icon = nullptr;
	FSlateColor IconColor = FSlateColor::UseForeground();
	FText ToolTip;
};

namespace ScriptableNamePickerItems
{
	/** Items for the Locals declared in Node's scope that pass Filter. Icon is the variable type pill. */
	TArray<FScriptableNameItem> FromLocals(const UObject* Node, TFunctionRef<bool(const FKzNamedVariant&)> Filter);

	/** Items for the functions of Class that pass Filter. Icon is the function glyph tinted by the return type's pin color. */
	TArray<FScriptableNameItem> FromFunctions(const UClass* Class, TFunctionRef<bool(const UFunction*)> Filter);
}

/**
 * FName dropdown whose menu replicates SPropertyBinding's: a searchable multibox menu with a
 * section heading and icon+name entry widgets, plus a Clear action while something is selected.
 * The item list is rebuilt through OnGetItems every time the menu opens, so it always reflects the
 * current scope (locals added or removed, the function list of a newly picked class, etc.).
 */
class SScriptableNamePicker : public SComboButton
{
public:
	DECLARE_DELEGATE_RetVal(TArray<FScriptableNameItem>, FOnGetItems);
	DECLARE_DELEGATE_OneParam(FOnNamePicked, FName);

	SLATE_BEGIN_ARGS(SScriptableNamePicker)
	{}
		/** Section heading shown above the entries (e.g. "Functions", "Locals"). The menu style renders it uppercase. */
		SLATE_ARGUMENT(FText, SectionTitle)
		/** Currently selected name, shown on the button with its icon. */
		SLATE_ATTRIBUTE(FName, CurrentName)
		/** Provides the pickable entries. Invoked on construction and every time the menu opens. */
		SLATE_EVENT(FOnGetItems, OnGetItems)
		/** Called when the user picks an entry (NAME_None for the Clear action). */
		SLATE_EVENT(FOnNamePicked, OnNamePicked)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedRef<SWidget> BuildMenuContent();
	void HandleNamePicked(FName PickedName);

	/** Rebuilds Items via OnGetItems (sorted). */
	void RefreshItems();

	/** Lookup of CurrentName in the cached items. Null when unset or no longer offered. */
	const FScriptableNameItem* FindCurrentItem() const;

	const FSlateBrush* GetCurrentIcon() const;
	FSlateColor GetCurrentIconColor() const;
	EVisibility GetCurrentIconVisibility() const;
	FText GetCurrentText() const;

	FText SectionTitle;
	TAttribute<FName> CurrentName;
	FOnGetItems OnGetItems;
	FOnNamePicked OnNamePicked;

	TArray<FScriptableNameItem> Items;
};
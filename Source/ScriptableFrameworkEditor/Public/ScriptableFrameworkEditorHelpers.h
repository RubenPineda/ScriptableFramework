// Copyright 2025 kirzo

#pragma once

#include "CoreMinimal.h"
#include "PropertyHandle.h"

class UScriptableObject;
struct FBindableStructDesc;

namespace ScriptableFrameworkEditor
{
	static const FName MD_SystemCategory = TEXT("System");
	static const FName MD_TaskCategory = TEXT("TaskCategory");
	static const FName MD_TaskCategories = TEXT("TaskCategories");
	static const FName MD_ConditionCategory = TEXT("ConditionCategory");
	static const FName MD_ConditionCategories = TEXT("ConditionCategories");

	bool IsPropertyVisible(TSharedRef<IPropertyHandle> PropertyHandle);
	void GetScriptableCategory(const UClass* ScriptableClass, FName& ClassCategoryMeta, FName& PropertyCategoryMeta);

	/** Determines if a specific property is exposed as an Output for binding. */
	bool IsPropertyBindableOutput(const FProperty* Property);

	/**
	 * Scans the hierarchy of the TargetObject (Parent, Grandparent, Previous Siblings)
	 * to find all objects that expose output properties for binding.
	 * @param TargetObject The object currently being edited/inspected.
	 * @param OutStructDescs The list of accessible bindable sources found.
	 */
	void GetAccessibleStructs(const UScriptableObject* TargetObject, TArray<FBindableStructDesc>& OutStructDescs);
}
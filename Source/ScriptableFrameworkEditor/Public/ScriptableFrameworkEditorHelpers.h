// Copyright 2025 kirzo

#pragma once

#include "CoreMinimal.h"
#include "PropertyHandle.h"

namespace ScriptableFrameworkEditor
{
	static const FName MD_SystemCategory = TEXT("System");
	static const FName MD_TaskCategory = TEXT("TaskCategory");
	static const FName MD_TaskCategories = TEXT("TaskCategories");
	static const FName MD_ConditionCategory = TEXT("ConditionCategory");
	static const FName MD_ConditionCategories = TEXT("ConditionCategories");

	bool IsPropertyVisible(TSharedRef<IPropertyHandle> PropertyHandle);
	void GetScriptableCategory(const UClass* ScriptableClass, FName& ClassCategoryMeta, FName& PropertyCategoryMeta);
}
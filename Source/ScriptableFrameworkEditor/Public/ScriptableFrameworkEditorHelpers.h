// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PropertyHandle.h"

namespace ScriptableFrameworkEditor
{
	static const FName MD_TaskCategory = TEXT("TaskCategory");
	static const FName MD_ConditionCategory = TEXT("ConditionCategory");

	bool IsPropertyVisible(TSharedRef<IPropertyHandle> PropertyHandle);
	FString GetScriptableCategory(const UClass* ScriptableClass);
}
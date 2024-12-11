// Copyright Kirzo. All Rights Reserved.

#include "ScriptableFrameworkEditorHelpers.h"
#include "PropertyHandle.h"

#include "ScriptableTasks/ScriptableTask.h"
#include "ScriptableConditions/ScriptableCondition.h"

namespace ScriptableFrameworkEditor
{
	bool IsPropertyVisible(TSharedRef<IPropertyHandle> PropertyHandle)
	{
		if (FProperty* Property = PropertyHandle->GetProperty())
		{
			if (Property->GetMetaData("Category") == TEXT("Hidden"))
			{
				return false;
			}

			// CPF_Edit and CPF_DisableEditOnInstance are automatically set on properties marked as 'EditDefaultsOnly'
			// See HeaderParser.cpp around line 3716 (switch case EVariableSpecifier::EditDefaultsOnly)
			if (Property->HasAllPropertyFlags(CPF_Edit | CPF_DisableEditOnInstance))
			{
				if (UObject* Object = Property->GetOwnerUObject())
				{
					// Properties of instanced UObjects marked as 'EditDefaultsOnly' are visible when the owner of the instance is an asset
					// This properties should only be visible in the CDO
					// Note: This also works with blueprint variables that are not 'InstanceEditable'
					return Object->IsTemplate();
				}
			}
		}
		return true;
	}

	void GetScriptableCategory(const UClass* ScriptableClass, FName& ClassCategoryMeta, FName& PropertyCategoryMeta)
	{
		if (ScriptableClass->IsChildOf(UScriptableTask::StaticClass()))
		{
			ClassCategoryMeta = MD_TaskCategory;
			PropertyCategoryMeta = MD_TaskCategories;
		}
		else if (ScriptableClass->IsChildOf(UScriptableCondition::StaticClass()))
		{
			ClassCategoryMeta = MD_ConditionCategory;
			PropertyCategoryMeta = MD_ConditionCategories;
		}
	}
}
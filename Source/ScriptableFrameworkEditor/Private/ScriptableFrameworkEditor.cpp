// Copyright Kirzo. All Rights Reserved.

#pragma once

#include "ScriptableFrameworkEditor.h"
#include "ScriptableTypeCache.h"

#include "ScriptableTasks/ScriptableTask.h"
#include "ScriptableConditions/ScriptableCondition.h"

#include "ScriptableFrameworkEd/Customization/ScriptableObjectCustomization.h"
#include "ScriptableFrameworkEd/Customization/ScriptableConditionCustomization.h"

#include "AssetToolsModule.h"

#define LOCTEXT_NAMESPACE "FScriptableFrameworkEditorModule"

EAssetTypeCategories::Type FScriptableFrameworkEditorModule::ScriptableFramework_AssetCategory = static_cast<EAssetTypeCategories::Type>(0);

void FScriptableFrameworkEditorModule::StartupModule()
{
	RegisterAssetActions();
	RegisterActorFactories();
	RegisterClassLayouts();
	RegisterPropertyLayouts();
}

void FScriptableFrameworkEditorModule::ShutdownModule()
{
	UnregisterAssetActions();
	UnregisterClassLayouts();
	UnregisterPropertyLayouts();
}

TSharedPtr<FScriptableTypeCache> FScriptableFrameworkEditorModule::GetScriptableTypeCache()
{
	if (!ScriptableTypeCache.IsValid())
	{
		ScriptableTypeCache = MakeShareable(new FScriptableTypeCache());
		ScriptableTypeCache->AddRootClass(UScriptableTask::StaticClass());
		ScriptableTypeCache->AddRootClass(UScriptableCondition::StaticClass());
	}

	return ScriptableTypeCache;
}

void FScriptableFrameworkEditorModule::RegisterAssetActions()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	// Register new asset category.
	ScriptableFramework_AssetCategory = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("ScriptableFramework")), LOCTEXT("ScriptableFramework_AssetCategory", "Scriptable Tasks"));
}

void FScriptableFrameworkEditorModule::UnregisterAssetActions()
{
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		for (const TSharedRef<IAssetTypeActions>& TypeAction : RegisteredAssetActions)
		{
			AssetTools.UnregisterAssetTypeActions(TypeAction);
		}
	}

	RegisteredAssetActions.Empty();
}

void FScriptableFrameworkEditorModule::RegisterActorFactories()
{
}

void FScriptableFrameworkEditorModule::RegisterClassLayouts()
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
}

void FScriptableFrameworkEditorModule::UnregisterClassLayouts()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		for (const FName ClassName : RegisteredClassLayouts)
		{
			PropertyEditorModule.UnregisterCustomClassLayout(ClassName);
		}
	}
}

void FScriptableFrameworkEditorModule::RegisterPropertyLayouts()
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	RegisterPropertyLayout<FScriptableObjectCustomization>(PropertyEditorModule, UScriptableTask::StaticClass()->GetFName());
	RegisterPropertyLayout<FScriptableConditionCustomization>(PropertyEditorModule, UScriptableCondition::StaticClass()->GetFName());
}

void FScriptableFrameworkEditorModule::UnregisterPropertyLayouts()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		for (const FName TypeName : RegisteredPropertyLayouts)
		{
			PropertyEditorModule.UnregisterCustomPropertyTypeLayout(TypeName);
		}
	}
}

template<typename T>
void FScriptableFrameworkEditorModule::RegisterActorFactory()
{
	auto ActorFactory = NewObject<T>();
	GEditor->ActorFactories.Add(ActorFactory);
}

template<typename TDetailsClass>
void FScriptableFrameworkEditorModule::RegisterClassLayout(FPropertyEditorModule& PropertyEditorModule, const FName ClassName)
{
	RegisteredClassLayouts.AddUnique(ClassName);
	PropertyEditorModule.RegisterCustomClassLayout(ClassName, FOnGetDetailCustomizationInstance::CreateStatic(&TDetailsClass::MakeInstance));
}

template<typename TPropertyType>
void FScriptableFrameworkEditorModule::RegisterPropertyLayout(FPropertyEditorModule& PropertyEditorModule, const FName TypeName)
{
	RegisteredPropertyLayouts.AddUnique(TypeName);
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(TypeName, FOnGetPropertyTypeCustomizationInstance::CreateStatic(&TPropertyType::MakeInstance));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FScriptableFrameworkEditorModule, ScriptableFrameworkEditor);
// Copyright Kirzo. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "AssetTypeCategories.h"

struct FScriptableClassCache;

class FScriptableFrameworkEditorModule : public IModuleInterface
{
public:
	static EAssetTypeCategories::Type ScriptableFramework_AssetCategory;

private:
	TArray<TSharedRef<class IAssetTypeActions>> RegisteredAssetActions;
	TArray<FName> RegisteredClassLayouts;
	TArray<FName> RegisteredPropertyLayouts;

public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	TSharedPtr<FScriptableClassCache> GetClassCache();

private:
	TSharedPtr<FScriptableClassCache> ClassCache;

	void RegisterAssetActions();
	void UnregisterAssetActions();

	void RegisterActorFactories();

	template<typename T>
	void RegisterActorFactory();

	void RegisterClassLayouts();
	void UnregisterClassLayouts();

	template<typename TDetailsClass>
	void RegisterClassLayout(FPropertyEditorModule& PropertyEditorModule, const FName ClassName);

	void RegisterPropertyLayouts();
	void UnregisterPropertyLayouts();

	template<typename TPropertyType>
	void RegisterPropertyLayout(FPropertyEditorModule& PropertyEditorModule, const FName TypeName);
};
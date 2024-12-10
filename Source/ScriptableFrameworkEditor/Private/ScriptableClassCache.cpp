// Copyright Epic Games, Inc. All Rights Reserved.

#include "ScriptableClassCache.h"
#include "Blueprint/BlueprintSupport.h"
#include "Misc/FeedbackContext.h"
#include "Modules/ModuleManager.h"
#include "UObject/UObjectIterator.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Blueprint.h"
#include "Editor.h"
#include "AssetRegistry/ARFilter.h"
#include "Logging/MessageLog.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/EnumerateRange.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ScriptableClassCache)

#define LOCTEXT_NAMESPACE "ScriptableFrameworkEditor"

FScriptableClassData::FScriptableClassData(UStruct* InStruct) :
	Struct(InStruct)
{
	if (InStruct)
	{
		StructName = InStruct->GetFName();
	}
}

FScriptableClassData::FScriptableClassData(const FString& InClassAssetName, const FString& InClassPackageName, const FName InStructName, UStruct* InStruct) :
	Struct(InStruct),
	StructName(InStructName),
	ClassAssetName(InClassAssetName),
	ClassPackageName(InClassPackageName)
{
}

UStruct* FScriptableClassData::GetStruct(bool bSilent)
{
	UStruct* Ret = Struct.Get();
	
	if (Ret == nullptr && ClassPackageName.Len())
	{
		GWarn->BeginSlowTask(LOCTEXT("LoadPackage", "Loading Package..."), true);

		UPackage* Package = LoadPackage(nullptr, *ClassPackageName, LOAD_NoRedirects);
		if (Package)
		{
			Package->FullyLoad();

			UObject* Object = FindObject<UObject>(Package, *ClassAssetName);

			GWarn->EndSlowTask();

			if (UBlueprint* BlueprintOb = Cast<UBlueprint>(Object))
			{
				Ret = *BlueprintOb->GeneratedClass;
			}
			else if (Object != nullptr)
			{
				Ret = Object->GetClass();
			}

			Struct = Ret;
		}
		else
		{
			GWarn->EndSlowTask();

			if (!bSilent)
			{
				FMessageLog EditorErrors("EditorErrors");
				EditorErrors.Error(LOCTEXT("PackageLoadFail", "Package Load Failed"));
				EditorErrors.Info(FText::FromString(ClassPackageName));
				EditorErrors.Notify(LOCTEXT("PackageLoadFail", "Package Load Failed"));
			}
		}
	}

	return Ret;
}

const UStruct* FScriptableClassData::GetInstanceDataStruct(bool bSilent /*= false*/)
{
	/*if (!InstanceDataStruct.IsValid())
	{
		if (const UScriptStruct* ScriptStruct = GetScriptStruct(bSilent))
		{
			TInstancedStruct<FStateTreeNodeBase> NodeInstance;
			NodeInstance.InitializeAsScriptStruct(ScriptStruct);

			InstanceDataStruct = NodeInstance.Get().GetInstanceDataType();
		}
	}*/

	return InstanceDataStruct.Get();
}

//----------------------------------------------------------------------//
//  FScriptableClassCache
//----------------------------------------------------------------------//
FScriptableClassCache::FScriptableClassCache()
{
	// Register with the Asset Registry to be informed when it is done loading up files.
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	AssetRegistryModule.Get().OnFilesLoaded().AddRaw(this, &FScriptableClassCache::InvalidateCache);
	AssetRegistryModule.Get().OnAssetAdded().AddRaw(this, &FScriptableClassCache::OnAssetAdded);
	AssetRegistryModule.Get().OnAssetRemoved().AddRaw(this, &FScriptableClassCache::OnAssetRemoved);

	// Register to have Populate called when doing a Reload.
	FCoreUObjectDelegates::ReloadCompleteDelegate.AddRaw(this, &FScriptableClassCache::OnReloadComplete);

	// Register to have Populate called when a Blueprint is compiled.
	check(GEditor);
	GEditor->OnBlueprintCompiled().AddRaw(this, &FScriptableClassCache::InvalidateCache);
	GEditor->OnClassPackageLoadedOrUnloaded().AddRaw(this, &FScriptableClassCache::InvalidateCache);
}

FScriptableClassCache::~FScriptableClassCache()
{
	// Unregister with the Asset Registry to be informed when it is done loading up files.
	if (FModuleManager::Get().IsModuleLoaded(TEXT("AssetRegistry")))
	{
		IAssetRegistry* AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).TryGet();
		if (AssetRegistry)
		{
			AssetRegistry->OnFilesLoaded().RemoveAll(this);
			AssetRegistry->OnAssetAdded().RemoveAll(this);
			AssetRegistry->OnAssetRemoved().RemoveAll(this);
		}

		// Unregister to have Populate called when doing a Reload.
		FCoreUObjectDelegates::ReloadCompleteDelegate.RemoveAll(this);

		// Unregister to have Populate called when a Blueprint is compiled.
		if (GEditor != nullptr)
		{
			// GEditor can't have been destructed before we call this or we'll crash.
			GEditor->OnBlueprintCompiled().RemoveAll(this);
			GEditor->OnClassPackageLoadedOrUnloaded().RemoveAll(this);
		}
	}
}

void FScriptableClassCache::AddRootStruct(UStruct* RootStruct)
{
	if (RootClasses.ContainsByPredicate([RootStruct](const FRootClassContainer& RootClass){ return RootClass.BaseStruct == RootStruct; }))
	{
		return;
	}
	
	RootClasses.Emplace(RootStruct);

	InvalidateCache();
}

void FScriptableClassCache::GetStructs(const UStruct* BaseStruct, TArray<TSharedPtr<FScriptableClassData>>& AvailableClasses)
{
	AvailableClasses.Reset();
	
	const int32 RootClassIndex = RootClasses.IndexOfByPredicate([BaseStruct](const FRootClassContainer& RootClass){ return RootClass.BaseStruct == BaseStruct; });
	if (RootClassIndex != INDEX_NONE)
	{
		const FRootClassContainer& RootClass = RootClasses[RootClassIndex];
		
		if (!RootClass.bUpdated)
		{
			CacheClasses();
		}

		AvailableClasses.Append(RootClass.ClassData);
	}
}

void FScriptableClassCache::OnAssetAdded(const FAssetData& AssetData)
{
	UpdateBlueprintClass(AssetData);
}

void FScriptableClassCache::OnAssetRemoved(const FAssetData& AssetData)
{
	FString AssetClassName;
	FString AssetNativeParentClassName;

	if (AssetData.GetTagValue(FBlueprintTags::GeneratedClassPath, AssetClassName) && AssetData.GetTagValue(FBlueprintTags::NativeParentClassPath, AssetNativeParentClassName))
	{
		ConstructorHelpers::StripObjectClass(AssetClassName);
		AssetClassName = FPackageName::ObjectPathToObjectName(AssetClassName);

		if (const int32* RootClassIndex = ClassNameToRootIndex.Find(AssetNativeParentClassName))
		{
			FRootClassContainer& RootClass = RootClasses[*RootClassIndex];
			const FName ClassName(AssetClassName);
			RootClass.ClassData.RemoveAll([&ClassName](const TSharedPtr<FScriptableClassData>& ClassData) { return ClassData->GetStructName() == ClassName; });
		}
	}
}

void FScriptableClassCache::InvalidateCache()
{
	for (FRootClassContainer& RootClass : RootClasses)
	{
		RootClass.ClassData.Reset();
		RootClass.bUpdated = false;
	}
	ClassNameToRootIndex.Reset();
}

void FScriptableClassCache::OnReloadComplete(EReloadCompleteReason Reason)
{
	InvalidateCache();
}

void FScriptableClassCache::UpdateBlueprintClass(const FAssetData& AssetData)
{
	FString AssetClassName;
	FString AssetNativeParentClassName;

	if (AssetData.GetTagValue(FBlueprintTags::GeneratedClassPath, AssetClassName) && AssetData.GetTagValue(FBlueprintTags::NativeParentClassPath, AssetNativeParentClassName))
	{
		UObject* AssetClassPackage = nullptr;
		ResolveName(AssetClassPackage, AssetClassName, false, false);

		UObject* AssetNativeParentClassPackage = nullptr;
		ResolveName(AssetNativeParentClassPackage, AssetNativeParentClassName, false, false);

		if (const int32* RootClassIndex = ClassNameToRootIndex.Find(AssetNativeParentClassName))
		{
			FRootClassContainer& RootClass = RootClasses[*RootClassIndex];

			const FName ClassName(AssetClassName);
			const int32 ClassDataIndex = RootClass.ClassData.IndexOfByPredicate([&ClassName](const TSharedPtr<FScriptableClassData>& ClassData) { return ClassData->GetStructName() == ClassName; });
			
			if (ClassDataIndex == INDEX_NONE)
			{
				UObject* AssetOb = AssetData.IsAssetLoaded() ? AssetData.GetAsset() : nullptr;
				UBlueprint* AssetBP = Cast<UBlueprint>(AssetOb);
				UClass* AssetClass = AssetBP ? *AssetBP->GeneratedClass : AssetOb ? AssetOb->GetClass() : nullptr;

				TSharedPtr<FScriptableClassData> NewData = MakeShareable(new FScriptableClassData(AssetData.AssetName.ToString(), AssetData.PackageName.ToString(), ClassName, AssetClass));
				RootClass.ClassData.Add(NewData);
			}
		}
	}
}

void FScriptableClassCache::CacheClasses()
{
	for (TEnumerateRef<FRootClassContainer> RootClass : EnumerateRange(RootClasses))
	{
		RootClass->ClassData.Reset();
		RootClass->bUpdated = true;

		// gather all native classes
		if (const UClass* Class = Cast<UClass>(RootClass->BaseStruct))
		{
			for (TObjectIterator<UClass> It; It; ++It)
			{
				UClass* TestClass = *It;
				if (TestClass->HasAnyClassFlags(CLASS_Native) && TestClass->IsChildOf(Class))
				{
					RootClass->ClassData.Add(MakeShareable(new FScriptableClassData(TestClass)));
					ClassNameToRootIndex.Add(TestClass->GetName(), RootClass.GetIndex());
				}
			}
		}
		else if (const UScriptStruct* ScriptStruct = Cast<UScriptStruct>(RootClass->BaseStruct))
		{
			for (TObjectIterator<UScriptStruct> It; It; ++It)
			{
				UScriptStruct* TestStruct = *It;
				if (TestStruct->IsChildOf(ScriptStruct))
				{
					RootClass->ClassData.Add(MakeShareable(new FScriptableClassData(TestStruct)));
					ClassNameToRootIndex.Add(TestStruct->GetName(), RootClass.GetIndex());
				}
			}
		}
	}

	// gather all blueprints
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> BlueprintList;

	FARFilter Filter;
	Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
	AssetRegistryModule.Get().GetAssets(Filter, BlueprintList);

	for (int32 i = 0; i < BlueprintList.Num(); i++)
	{
		UpdateBlueprintClass(BlueprintList[i]);
	}
}

#undef LOCTEXT_NAMESPACE
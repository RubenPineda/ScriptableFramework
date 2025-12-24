// Copyright 2025 kirzo

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class FAssetTypeActions_ScriptableObjectBase : public FAssetTypeActions_Base
{
public:
	explicit FAssetTypeActions_ScriptableObjectBase(EAssetTypeCategories::Type AssetCategory)
		: AssetCategory(AssetCategory)
	{
	}

	virtual uint32 GetCategories() override { return AssetCategory; }
	
private:
	EAssetTypeCategories::Type AssetCategory;
};

class FAssetTypeActions_ScriptableObject : public FAssetTypeActions_ScriptableObjectBase
{
public:
	const FText Name;
	const FColor Color;
	UClass* const SupportedClass;

	FAssetTypeActions_ScriptableObject(EAssetTypeCategories::Type AssetCategory, const FText& Name, FColor Color, UClass* SupportedClass)
		: FAssetTypeActions_ScriptableObjectBase(AssetCategory)
		, Name(Name)
		, Color(Color)
		, SupportedClass(SupportedClass)
	{
	}

	virtual FText GetName() const override { return Name; }
	virtual FColor GetTypeColor() const override { return Color; }
	virtual UClass* GetSupportedClass() const override { return SupportedClass; }
};
// Copyright 2025 kirzo

#include "Factories/ScriptableConditionAssetFactory.h"
#include "ScriptableConditions/ScriptableConditionAsset.h"

UScriptableConditionAssetFactory::UScriptableConditionAssetFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UScriptableConditionAsset::StaticClass();
}

UObject* UScriptableConditionAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UScriptableConditionAsset>(InParent, Class, Name, Flags | RF_Transactional);
}
// Copyright 2025 kirzo

#include "Factories/ScriptableTaskAssetFactory.h"
#include "ScriptableTasks/ScriptableTaskAsset.h"

UScriptableTaskAssetFactory::UScriptableTaskAssetFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UScriptableTaskAsset::StaticClass();
}

UObject* UScriptableTaskAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UScriptableTaskAsset>(InParent, Class, Name, Flags | RF_Transactional);
}
// Copyright 2026 kirzo

#include "Factories/ScriptableGraphAssetFactory.h"
#include "ScriptableNodes/ScriptableGraph.h"

UScriptableGraphAssetFactory::UScriptableGraphAssetFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UScriptableGraph::StaticClass();
}

UObject* UScriptableGraphAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UScriptableGraph>(InParent, Class, Name, Flags | RF_Transactional);
}
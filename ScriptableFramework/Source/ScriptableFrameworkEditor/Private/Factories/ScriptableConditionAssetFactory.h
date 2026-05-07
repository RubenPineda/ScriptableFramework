// Copyright 2025 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "ScriptableConditionAssetFactory.generated.h"

UCLASS()
class UScriptableConditionAssetFactory : public UFactory
{
	GENERATED_BODY()

public:
	UScriptableConditionAssetFactory();

	// UFactory interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	// End of UFactory interface
};
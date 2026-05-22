// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "ScriptableGraphAssetFactory.generated.h"

UCLASS()
class UScriptableGraphAssetFactory : public UFactory
{
	GENERATED_BODY()

public:
	UScriptableGraphAssetFactory();

	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
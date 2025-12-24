// Copyright 2025 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "ScriptableTaskAssetFactory.generated.h"

UCLASS()
class UScriptableTaskAssetFactory : public UFactory
{
	GENERATED_BODY()

public:
	UScriptableTaskAssetFactory();

	// UFactory interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	// End of UFactory interface
};
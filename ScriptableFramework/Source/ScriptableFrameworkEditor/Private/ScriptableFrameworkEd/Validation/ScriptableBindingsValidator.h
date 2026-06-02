// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Validation/KzAssetValidator.h"
#include "ScriptableBindingsValidator.generated.h"

/**
 * Binding validator for UScriptableGraph assets. Surfaces input/context binding errors
 * inside the graph editor's Validation panel with click-to-navigate via node BindingID.
 * Runs alongside UScriptableGraphValidator; both are discovered by FKzAssetValidationUtils.
 */
UCLASS()
class UScriptableBindingsValidator : public UKzAssetValidator
{
	GENERATED_BODY()

public:
	//~ UKzAssetValidator interface
	virtual FName GetValidatorId_Implementation() const override;
	virtual bool CanValidate_Implementation(const UObject* Asset) const override;
	virtual void Validate_Implementation(const UObject* Asset, TArray<FKzValidationIssue>& OutIssues) const override;
	//~ End of UKzAssetValidator interface
};

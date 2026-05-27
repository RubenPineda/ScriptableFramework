// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Validation/KzAssetValidator.h"
#include "ScriptableGraphValidator.generated.h"

/**
 * Structural validator for UScriptableGraph assets. Reports unreachable nodes, broken/inverted
 * connections, cycles, RunGraph self/indirect recursion, and empty ReceiveEvent names. Runs
 * alongside the binding validator; discovered automatically by FKzAssetValidationUtils.
 */
UCLASS()
class UScriptableGraphValidator : public UKzAssetValidator
{
	GENERATED_BODY()

public:
	//~ UKzAssetValidator interface
	virtual FName GetValidatorId_Implementation() const override;
	virtual bool CanValidate_Implementation(const UObject* Asset) const override;
	virtual void Validate_Implementation(const UObject* Asset, TArray<FKzValidationIssue>& OutIssues) const override;
	//~ End of UKzAssetValidator interface
};

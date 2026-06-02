// Copyright 2026 kirzo

#include "ScriptableFrameworkValidator.h"
#include "ScriptableFrameworkEd/Validation/ScriptableBindingsValidation.h"
#include "ScriptableNodes/ScriptableGraph.h"

#include "Misc/DataValidation.h"

#define LOCTEXT_NAMESPACE "ScriptableFrameworkValidator"

UScriptableFrameworkValidator::UScriptableFrameworkValidator()
{
	bIsEnabled = true;
}

bool UScriptableFrameworkValidator::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& InContext) const
{
	/** UScriptableGraph is validated by its editor panel via UKzAssetValidator — skip here to avoid duplicate warnings. */
	if (InAsset && InAsset->IsA<UScriptableGraph>()) return false;
	return InAsset != nullptr;
}

EDataValidationResult UScriptableFrameworkValidator::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& InContext)
{
	TArray<FKzValidationIssue> Issues;
	FScriptableBindingsValidation::ValidateBindings(InAsset, Issues);

	if (Issues.Num() == 0)
	{
		AssetPasses(InAsset);
		return EDataValidationResult::Valid;
	}

	for (const FKzValidationIssue& Issue : Issues)
	{
		const EMessageSeverity::Type Severity =
			Issue.Severity == EKzValidationSeverity::Error ? EMessageSeverity::Error :
			Issue.Severity == EKzValidationSeverity::Warning ? EMessageSeverity::Warning :
			EMessageSeverity::Info;

		const FText Prefixed = FText::Format(LOCTEXT("AssetIssueFmt", "{0}: {1}"), FText::FromName(InAssetData.AssetName), Issue.Message);
		AssetMessage(InAssetData, Severity, Prefixed);
	}

	return EDataValidationResult::Invalid;
}

#undef LOCTEXT_NAMESPACE

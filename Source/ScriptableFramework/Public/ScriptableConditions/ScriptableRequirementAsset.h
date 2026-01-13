// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableObjectAsset.h"
#include "ScriptableRequirement.h"
#include "ScriptableCondition.h"
#include "ScriptableRequirementAsset.generated.h"

/** An asset that defines a reusable Requirement. */
UCLASS(BlueprintType, Const)
class SCRIPTABLEFRAMEWORK_API UScriptableRequirementAsset final : public UScriptableObjectAsset
{
	GENERATED_BODY()

public:
	/** The requirement definition. Holds the Context variables and the list of Conditions. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
	FScriptableRequirement Requirement;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};

UCLASS(EditInlineNew, BlueprintType, NotBlueprintable, meta = (DisplayName = "Evaluate Asset", Hidden))
class SCRIPTABLEFRAMEWORK_API UScriptableCondition_Asset final : public UScriptableCondition
{
	GENERATED_BODY()

public:
	/** The asset containing the Requirement definition to evaluate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	TObjectPtr<UScriptableRequirementAsset> Asset;

	// --- Lifecycle ---
	virtual void OnRegister() override;
	virtual void OnUnregister() override;

#if WITH_EDITOR
	virtual FText GetDescription() const override;
#endif

protected:
	virtual bool Evaluate_Implementation() const override;

private:
	/** The actual instance created from the asset template. */
	UPROPERTY(Transient)
	TObjectPtr<UScriptableCondition> Condition;
};
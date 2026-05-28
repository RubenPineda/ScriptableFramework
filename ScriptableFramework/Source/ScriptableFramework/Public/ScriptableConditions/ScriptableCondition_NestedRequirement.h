// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableConditions/ScriptableCondition.h"
#include "ScriptableConditions/ScriptableRequirement.h"
#include "ScriptableCondition_NestedRequirement.generated.h"

/**
 * A grouping condition that wraps a full FScriptableRequirement so you can express nested logic
 * (e.g. "(A AND B) OR (C AND D)") directly inside a parent requirement. Inherits its parent's
 * context — it does not declare its own scope.
 */
UCLASS(meta = (DisplayName = "Nested Requirement", ConditionCategory = "System"))
class SCRIPTABLEFRAMEWORK_API UScriptableCondition_NestedRequirement : public UScriptableCondition
{
	GENERATED_BODY()

public:
	/** Optional descriptive label rendered in the row header (e.g. "Ammo Checks"). */
	UPROPERTY(EditAnywhere, Category = "Nested Requirement", meta = (NoBinding))
	FString RequirementName;

	/** The nested logic block (Conditions + Mode). Context is intentionally inherited from the parent scope. */
	UPROPERTY(EditAnywhere, Category = "Nested Requirement", meta = (ShowOnlyInnerProperties, NoBinding))
	FScriptableRequirement Requirement;

	//~ UScriptableObject interface
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	//~ End of UScriptableObject interface

protected:
	virtual bool Evaluate_Implementation() const override;
};

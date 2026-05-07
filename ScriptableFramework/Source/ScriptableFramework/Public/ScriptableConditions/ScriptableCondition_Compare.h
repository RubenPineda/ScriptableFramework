// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableCondition.h"
#include "ScriptableCondition_Compare.generated.h"

UENUM(BlueprintType)
enum class EScriptableComparisonOp : uint8
{
	Equal           UMETA(DisplayName = "=="),
	NotEqual        UMETA(DisplayName = "!="),
	Less            UMETA(DisplayName = "<"),
	LessOrEqual     UMETA(DisplayName = "<="),
	Greater         UMETA(DisplayName = ">"),
	GreaterOrEqual  UMETA(DisplayName = ">=")
};

/** System condition to compare two numbers (Floats, Ints, Doubles). */
UCLASS(DisplayName = "Compare Numbers", meta = (ConditionCategory = "System|Math"))
class SCRIPTABLEFRAMEWORK_API UScriptableCondition_CompareNumbers : public UScriptableCondition
{
	GENERATED_BODY()

public:
	/** The first value to compare */
	UPROPERTY(EditAnywhere, Category = "Config")
	double A = 0.0;

	/** The second value to compare */
	UPROPERTY(EditAnywhere, Category = "Config")
	double B = 0.0;

	/** The comparison operator */
	UPROPERTY(EditAnywhere, Category = "Config")
	EScriptableComparisonOp Operation = EScriptableComparisonOp::Equal;

	/** Error tolerance for float equality checks. Only used for == and != */
	UPROPERTY(EditAnywhere, Category = "Config", meta = (EditCondition = "Operation == EScriptableComparisonOp::Equal || Operation == EScriptableComparisonOp::NotEqual", EditConditionHides))
	double ErrorTolerance = 1.e-4;

#if WITH_EDITOR
	virtual FText GetDisplayTitle() const override;
#endif

protected:
	virtual bool Evaluate_Implementation() const override;
};

/** Checks the distance between two Actors. */
UCLASS(DisplayName = "Distance Check", meta = (ConditionCategory = "System|Spatial"))
class SCRIPTABLEFRAMEWORK_API UScriptableCondition_Distance : public UScriptableCondition
{
	GENERATED_BODY()

public:
	/** The origin actor. */
	UPROPERTY(EditAnywhere, Category = "Config")
	TObjectPtr<AActor> Origin = nullptr;

	/** The target actor. */
	UPROPERTY(EditAnywhere, Category = "Config")
	TObjectPtr<AActor> Target = nullptr;

	/** The comparison operator (e.g., <, >, ==). */
	UPROPERTY(EditAnywhere, Category = "Config")
	EScriptableComparisonOp Operation = EScriptableComparisonOp::Less;

	/** The distance threshold to compare against. */
	UPROPERTY(EditAnywhere, Category = "Config")
	float Distance = 500.0f;

#if WITH_EDITOR
	virtual FText GetDisplayTitle() const override;
#endif

protected:
	virtual bool Evaluate_Implementation() const override;
};
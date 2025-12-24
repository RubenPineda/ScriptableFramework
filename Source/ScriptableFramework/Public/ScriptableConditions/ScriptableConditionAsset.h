// Copyright 2025 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableConditionAsset.generated.h"

UCLASS(BlueprintType, Const)
class SCRIPTABLEFRAMEWORK_API UScriptableConditionAsset : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = Condition, meta = (ShowOnlyInnerProperties))
	class UScriptableCondition* Condition;
};
// Copyright 2025 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableTaskAsset.generated.h"

UCLASS(BlueprintType, Const)
class SCRIPTABLEFRAMEWORK_API UScriptableTaskAsset : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = ScriptableTask, meta = (ShowOnlyInnerProperties))
	class UScriptableTask* Task;
};
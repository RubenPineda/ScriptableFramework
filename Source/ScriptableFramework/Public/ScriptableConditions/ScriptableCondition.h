// Copyright 2025 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableObject.h"
#include "ScriptableCondition.generated.h"

UCLASS(Abstract, DefaultToInstanced, EditInlineNew, Blueprintable, BlueprintType, HideCategories = (Hidden, Tick), CollapseCategories)
class SCRIPTABLEFRAMEWORK_API UScriptableCondition : public UScriptableObject
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, Category = Hidden)
	uint8 bNegate : 1 = 0;

public:
	FORCEINLINE bool IsNegated() const { return bNegate; }

	virtual bool CanEverTick() const final override { return false; }

protected:
	UFUNCTION(BlueprintNativeEvent, Category = ScriptableCondition)
	bool Evaluate() const;

private:
	virtual bool Evaluate_Implementation() const { return false; }

public:
	UFUNCTION(BlueprintCallable, Category = ScriptableCondition, meta = (DefaultToSelf = "Owner"))
	static bool EvaluateCondition(UObject* Owner, UScriptableCondition* Condition);
};

UCLASS(EditInlineNew, BlueprintType, NotBlueprintable, meta = (DisplayName = "AND", ConditionCategory = "System"))
class UScriptableCondition_AND final : public UScriptableCondition
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = Condition, meta = (ShowOnlyInnerProperties))
	TArray<UScriptableCondition*> Conditions;

private:
	virtual bool Evaluate_Implementation() const override;
};

UCLASS(EditInlineNew, BlueprintType, NotBlueprintable, meta = (DisplayName = "OR", ConditionCategory = "System"))
class UScriptableCondition_OR final : public UScriptableCondition
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = Condition, meta = (ShowOnlyInnerProperties))
	TArray<UScriptableCondition*> Conditions;

private:
	virtual bool Evaluate_Implementation() const override;
};

UCLASS(EditInlineNew, BlueprintType, NotBlueprintable, meta = (DisplayName = "Asset", ConditionCategory = "System"))
class UScriptableCondition_Asset final : public UScriptableCondition
{
	GENERATED_BODY()

public:
	/** The asset containing the condition definition to evaluate. */
	UPROPERTY(EditAnywhere, Category = "Condition")
	class UScriptableConditionAsset* AssetToEvaluate;

protected:
	/** The actual instance created from the asset template. */
	UPROPERTY(Transient)
	UScriptableCondition* Condition;

public:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;

private:
	virtual bool Evaluate_Implementation() const override;

	void CreateConditionInstance();
	void ClearConditionInstance();
};
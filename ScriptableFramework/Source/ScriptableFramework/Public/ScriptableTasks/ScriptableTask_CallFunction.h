// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableTasks/ScriptableTask.h"
#include "StructUtils/PropertyBag.h"
#include "Templates/SubclassOf.h"
#include "ScriptableTask_CallFunction.generated.h"

struct FKzNamedVariant;

/**
 * Calls a BlueprintCallable function on a target object via reflection.
 * Complements pure-function binding sources: use this for functions with side effects.
 * Parameters mirrors the function's signature; each member holds a constant unless bound.
 * The return value, if any, can be written into a Local for downstream nodes to read.
 * Synchronous only; functions with out or by-ref parameters are not offered.
 */
UCLASS(DisplayName = "Call Function", meta = (TaskCategory = "System|Object"))
class SCRIPTABLEFRAMEWORK_API UScriptableTask_CallFunction : public UScriptableTask
{
	GENERATED_BODY()

public:
	/** Object to call the function on. Must be an instance of TargetClass at runtime. */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UObject> Target;

	/** Class the function is picked from. */
	UPROPERTY(EditAnywhere, Category = "Config", meta = (NoBinding))
	TSubclassOf<UObject> TargetClass;

	/** Function to call. Lists the BlueprintCallable functions of TargetClass. */
	UPROPERTY(EditAnywhere, Category = "Config", meta = (NoBinding, GetOptions = "GetFunctionNames"))
	FName FunctionName = NAME_None;

	/** Call arguments. Shape mirrors the function signature; set constants inline or bind members. */
	UPROPERTY(EditAnywhere, Category = "Config", meta = (FixedLayout))
	FInstancedPropertyBag Parameters;

	/** Optional Local that receives the function's return value. Only type-compatible Locals are offered. */
	UPROPERTY(EditAnywhere, Category = "Config", meta = (NoBinding, GetOptions = "GetLocalNames"))
	FName ResultLocal = NAME_None;

	virtual bool IsStoppable() const override { return false; }

protected:
	virtual void BeginTask() override;

private:
	void WriteReturnValueToLocal(const UFunction* Function, uint8* FrameMemory);

#if WITH_EDITOR
public:
	virtual FText GetDisplayTitle() const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PreSave(FObjectPreSaveContext SaveContext) override;
	virtual void ValidateObject(TArray<FKzValidationIssue>& OutIssues) const override;

	/** True if Func can be offered: BlueprintCallable, non-static, no out/ref params, bag-representable inputs. */
	static bool IsFunctionExposable(const UFunction* Func);

	/** True if Local's declared type can receive Function's return value (exact, numeric or related object classes). */
	static bool CanLocalReceiveReturnValue(const UFunction* Function, const FKzNamedVariant& Local);

private:
	UFUNCTION()
	TArray<FString> GetFunctionNames() const;

	UFUNCTION()
	TArray<FString> GetLocalNames() const;

	/** Returns the bag layout matching Function's current input parameters, or null for a parameterless function. */
	static const UPropertyBag* BuildParametersBagStruct(const UFunction* Function);

	/** True when BagStruct's members match Function's input parameters by name and type. Compares shape only: desc IDs and metadata differ across save/load, so bag struct pointers cannot be compared between sessions. */
	static bool AreParametersInSync(const UPropertyBag* BagStruct, const UFunction* Function);

	/** Rebuilds Parameters to mirror the current function signature, preserving overlapping values. */
	void SyncParametersBag();

	/** Clears ResultLocal when it no longer names a Local compatible with the current function's return type. */
	void SanitizeResultLocal();
#endif
};
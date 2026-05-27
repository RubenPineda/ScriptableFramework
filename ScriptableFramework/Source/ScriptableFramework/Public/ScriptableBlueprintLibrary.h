// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ScriptableTasks/ScriptableAction.h"
#include "ScriptableConditions/ScriptableRequirement.h"
#include "ScriptableContext.h"
#include "Core/KzTypeDef.h"
#include "ScriptableBlueprintLibrary.generated.h"

class UScriptableGraphInstance;

UCLASS()
class SCRIPTABLEFRAMEWORK_API UScriptableBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Adds a context parameter definition by name, inferring the type from the supplied default value.
	 * @param Context        The scriptable context to modify.
	 * @param ParameterName  The name of the parameter.
	 * @param DefaultValue   The default value.
	 */
	UFUNCTION(BlueprintCallable, Category = "Scriptable Framework|Context", meta = (AutoCreateRefTerm = "Type"))
	static void AddScriptableContextProperty(UPARAM(Ref) FScriptableContext& Context, FName ParameterName, const FKzTypeDef& Type);

	/**
	 * Sets a context parameter value by name.
	 * @param Context        The scriptable context to modify.
	 * @param ParameterName  The name of the parameter.
	 * @param Value          The value to set..
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, Category = "Scriptable Framework|Context", meta = (CustomStructureParam = "Value", AutoCreateRefTerm = "Value"))
	static void SetScriptableContextProperty(UPARAM(Ref) FScriptableContext& Context, FName ParameterName, const int32& Value);

	/**
	 * Evaluates a Scriptable Requirement.
	 * Registers the requirement against the given owner, evaluates it, and unregisters it before returning.
	 * @param Owner Object responsible for evaluation context (typically 'self').
	 * @param Requirement The requirement to evaluate.
	 * @return True if the requirement passes (respecting Mode and bNegate).
	 */
	UFUNCTION(BlueprintCallable, Category = "Scriptable Framework|Requirement", meta = (DefaultToSelf = "Owner", Keywords = "evaluate check requirement condition"))
	static bool EvaluateRequirement(UObject* Owner, const FScriptableRequirement& Requirement);

	/**
	 * Evaluates a Scriptable Requirement using values from an external context.
	 * The context's values replace the requirement's internal bag values for this evaluation.
	 * @param Owner       Object responsible for evaluation context (typically 'self').
	 * @param Requirement The requirement to evaluate.
	 * @param Context     The context supplying the values.
	 * @return True if the requirement passes (respecting Mode and bNegate).
	 */
	UFUNCTION(BlueprintCallable, Category = "Scriptable Framework|Requirement", meta = (DefaultToSelf = "Owner", Keywords = "evaluate check requirement condition context"))
	static bool EvaluateRequirementWithContext(UObject* Owner, const FScriptableRequirement& Requirement, const FScriptableContext& Context);

	/**
	 * Sets a context parameter value by name.
	 * @param Action         The scriptable action to modify.
	 * @param ParameterName  The name of the parameter (must match ContextDefinitions).
	 * @param Value          The value to set. Type must match the definition.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, Category = "Scriptable Framework|Context", meta = (CustomStructureParam = "Value", AutoCreateRefTerm = "Value"))
	static void SetActionContextParameter(UPARAM(Ref) FScriptableAction& Action, FName ParameterName, const int32& Value);

	/**
	 * Sets a context parameter value by name.
	 * @param Requirement    The scriptable requirement to modify.
	 * @param ParameterName  The name of the parameter (must match ContextDefinitions).
	 * @param Value          The value to set. Type must match the definition.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, Category = "Scriptable Framework|Context", meta = (CustomStructureParam = "Value", AutoCreateRefTerm = "Value"))
	static void SetRequirementContextParameter(UPARAM(Ref) FScriptableRequirement& Requirement, FName ParameterName, const int32& Value);

	/**
	 * Sets a context parameter value on a live graph runner.
	 * @param Runner         The live UScriptableGraphInstance whose context to mutate. Must be valid.
	 * @param ParameterName  The name of the parameter.
	 * @param Value          The value to set. Type wildcard.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, Category = "Scriptable Framework|Context", meta = (CustomStructureParam = "Value", AutoCreateRefTerm = "Value"))
	static void SetGraphInstanceContextProperty(UScriptableGraphInstance* Runner, FName ParameterName, const int32& Value);

	/**
	 * Copies a context's values into an action's internal bag for matching property names.
	 * @param Action   The scriptable action to modify.
	 * @param Context  The context supplying the values.
	 */
	UFUNCTION(BlueprintCallable, Category = "Scriptable Framework|Context")
	static void SetActionContext(UPARAM(Ref) FScriptableAction& Action, const FScriptableContext& Context);

	/**
	 * Copies a context's values into a requirement's internal bag for matching property names.
	 * @param Requirement  The scriptable requirement to modify.
	 * @param Context      The context supplying the values.
	 */
	UFUNCTION(BlueprintCallable, Category = "Scriptable Framework|Context")
	static void SetRequirementContext(UPARAM(Ref) FScriptableRequirement& Requirement, const FScriptableContext& Context);

private:
	DECLARE_FUNCTION(execSetScriptableContextProperty);
	DECLARE_FUNCTION(execSetActionContextParameter);
	DECLARE_FUNCTION(execSetRequirementContextParameter);
	DECLARE_FUNCTION(execSetGraphInstanceContextProperty);
};
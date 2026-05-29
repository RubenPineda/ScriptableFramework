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
	 * Returns the same context so several builder nodes can be chained.
	 * @param Context        The scriptable context to modify.
	 * @param ParameterName  The name of the parameter.
	 * @param Type           The parameter type.
	 */
	UFUNCTION(BlueprintCallable, Category = "Scriptable Framework|Context", meta = (AutoCreateRefTerm = "Type", ReturnDisplayName = "Context"))
	static FScriptableContext AddScriptableContextProperty(UPARAM(Ref) FScriptableContext& Context, FName ParameterName, const FKzTypeDef& Type);

	/**
	 * Sets a context parameter value by name. Returns the same context so several setters can be chained.
	 * @param Context        The scriptable context to modify.
	 * @param ParameterName  The name of the parameter.
	 * @param Value          The value to set.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, Category = "Scriptable Framework|Context", meta = (CustomStructureParam = "Value", AutoCreateRefTerm = "Value", ReturnDisplayName = "Context"))
	static FScriptableContext SetScriptableContextProperty(UPARAM(Ref) FScriptableContext& Context, FName ParameterName, const int32& Value);

	/**
	 * Creates a context holding a single named parameter, typed from the supplied value. Chain
	 * SetScriptableContextProperty / AddScriptableContextProperty onto its output to add more.
	 * @param ParameterName  The name of the parameter.
	 * @param Value          The value to store (defines the parameter's type).
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, Category = "Scriptable Framework|Context", meta = (CustomStructureParam = "Value", AutoCreateRefTerm = "Value", ReturnDisplayName = "Context"))
	static FScriptableContext MakeScriptableContext(FName ParameterName, const int32& Value);

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

	/**
	 * Cancels every live scriptable runner (graphs + actions) in the world. Graph runners run their Exit
	 * cleanup sub-flow if they declare one; action runners force-finish. No-op if the world has no subsystem.
	 * @param WorldContext  Any object resolvable to a world (auto-filled by Blueprint).
	 */
	UFUNCTION(BlueprintCallable, Category = "Scriptable Framework|Runners", meta = (WorldContext = "WorldContext"))
	static void CancelAllScriptableRunners(const UObject* WorldContext);

	/**
	 * Cancels every live scriptable runner (graphs + actions) whose Launch owner matches the supplied
	 * UObject. Same Cancel semantics as CancelAllScriptableRunners. No-op if Owner is null.
	 * @param WorldContext  Any object resolvable to a world (auto-filled by Blueprint).
	 * @param Owner         The UObject the runners were launched with (defaults to Self in Blueprint).
	 */
	UFUNCTION(BlueprintCallable, Category = "Scriptable Framework|Runners", meta = (WorldContext = "WorldContext", DefaultToSelf = "Owner"))
	static void CancelScriptableRunnersForOwner(const UObject* WorldContext, UObject* Owner);

private:
	DECLARE_FUNCTION(execSetScriptableContextProperty);
	DECLARE_FUNCTION(execMakeScriptableContext);
	DECLARE_FUNCTION(execSetActionContextParameter);
	DECLARE_FUNCTION(execSetRequirementContextParameter);
	DECLARE_FUNCTION(execSetGraphInstanceContextProperty);
};
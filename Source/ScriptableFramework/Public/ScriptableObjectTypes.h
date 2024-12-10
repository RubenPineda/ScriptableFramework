// Copyright Kirzo. All Rights Reserved.

#pragma once

/**
*	This file is for shared structs and enums
*/

#include "Engine/EngineBaseTypes.h"
#include "ScriptableObjectTypes.generated.h"

class UScriptableObject;

/** 
* Tick function that calls UScriptableObject::Tick
*/
USTRUCT()
struct FScriptableObjectTickFunction : public FTickFunction
{
	GENERATED_BODY()

	/**  Scriptable Object that is the target of this tick **/
	class UScriptableObject* Target;

	/** 
	* Abstract function actually execute the tick. 
	* @param DeltaTime - frame time to advance, in seconds
	* @param TickType - kind of tick for this frame
	* @param CurrentThread - thread we are executing on, useful to pass along as new objects are created
	* @param MyCompletionGraphEvent - completion event for this object. Useful for holding the completetion of this object until certain child objects are complete.
	*/
	SCRIPTABLEFRAMEWORK_API virtual void ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override;
	/** Abstract function to describe this tick. Used to print messages about illegal cycles in the dependency graph **/
	SCRIPTABLEFRAMEWORK_API virtual FString DiagnosticMessage() override;
	SCRIPTABLEFRAMEWORK_API virtual FName DiagnosticContext(bool bDetailed) override;

	/**
	* Conditionally calls ExecuteTickFunc if registered and a bunch of other criteria are met
	* @param Target - the scriptable object we are ticking
	* @param bTickInEditor - whether the target wants to tick in the editor
	* @param DeltaTime - The time since the last tick.
	* @param TickType - Type of tick that we are running
	* @param ExecuteTickFunc - the lambda that ultimately calls tick on the scriptable object
	*/
	template <typename ExecuteTickLambda>
	static void ExecuteTickHelper(UScriptableObject* Target, bool bTickInEditor, float DeltaTime, ELevelTick TickType, const ExecuteTickLambda& ExecuteTickFunc);
};

template<>
struct TStructOpsTypeTraits<FScriptableObjectTickFunction> : public TStructOpsTypeTraitsBase2<FScriptableObjectTickFunction>
{
	enum
	{
		WithCopy = false
	};
};
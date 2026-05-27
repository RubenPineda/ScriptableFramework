// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ScriptableGraphSubsystem.generated.h"

class UScriptableGraph;
class UScriptableGraphInstance;
struct FScriptableContext;

/**
 * World-scoped registry of live UScriptableGraphInstance runners. Holding the runners here keeps them
 * alive while they execute (replacing the old self-reference anchor) and lets the world cancel them all
 * on teardown (PIE end, level change) so callbacks never fire against destroyed actors.
 */
UCLASS()
class SCRIPTABLEFRAMEWORK_API UScriptableGraphSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Canonical entry point: creates a runner, registers it, and launches it. Returns null if the world can't be resolved. */
	static UScriptableGraphInstance* RunGraph(const UObject* WorldContext, UScriptableGraph* Asset, UObject* Owner, const FScriptableContext& Context);

	/** Resolves the subsystem from any world context object, or null if there is no world. */
	static UScriptableGraphSubsystem* Get(const UObject* WorldContext);

	/** Cancels every live runner. Called on Deinitialize; also callable directly. */
	UFUNCTION(BlueprintCallable, Category = "Scriptable Framework|Graph")
	void CancelAllRunners();

	/** Returns a copy of the currently live runners. Useful for debugging. */
	UFUNCTION(BlueprintCallable, Category = "Scriptable Framework|Graph")
	TArray<UScriptableGraphInstance*> GetActiveRunners() const;

	//~ USubsystem interface
	virtual void Deinitialize() override;
	//~ End of USubsystem interface

private:
	/** Adds a runner to the live set (idempotent). Called by the runner from Launch. */
	void RegisterRunner(UScriptableGraphInstance* Runner);

	/** Removes a runner from the live set. Called by the runner from Finish (and defensively on destroy). */
	void UnregisterRunner(UScriptableGraphInstance* Runner);

	/** Live runners. These strong refs are what keep runners alive while they execute. */
	UPROPERTY()
	TArray<TObjectPtr<UScriptableGraphInstance>> ActiveRunners;

	friend class UScriptableGraphInstance;
};

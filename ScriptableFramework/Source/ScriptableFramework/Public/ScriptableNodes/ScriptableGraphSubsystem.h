// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ScriptableGraphSubsystem.generated.h"

class UScriptableGraph;
class UScriptableGraphInstance;
class UScriptableActionRunner;
struct FScriptableContext;

/**
 * World-scoped registry of live scriptable runners (graph instances and action runners). Holding them
 * here keeps them alive while they execute (replacing the old self-reference anchor) and lets the world
 * cancel them all on teardown (PIE end, level change) in a safe context before GC — so their task
 * Stop/finish logic never fires against destroyed actors or, worse, during garbage collection.
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

	/** User-invoked cancel of every live runner (graphs + actions). Graph runners run their Exit cleanup
	 * sub-flow if they declare one; action runners force-finish. For world-teardown cancel (skip Exit),
	 * the subsystem uses a private path in Deinitialize. */
	UFUNCTION(BlueprintCallable, Category = "Scriptable Framework|Graph")
	void CancelAllRunners();

	/** Cancels every live runner whose Launch owner matches the supplied UObject. Same Cancel semantics
	 * as CancelAllRunners: graphs run Exit cleanup if present, actions force-finish. No-op if Owner is null. */
	UFUNCTION(BlueprintCallable, Category = "Scriptable Framework|Graph")
	void CancelRunnersForOwner(UObject* Owner);

	/** Returns a copy of the currently live runners. Useful for debugging. */
	UFUNCTION(BlueprintCallable, Category = "Scriptable Framework|Graph")
	TArray<UScriptableGraphInstance*> GetActiveRunners() const;

	/** Cancel a specific runner. Same semantics as CancelAllRunners (Exit cleanup if declared). No-op if null or already finished. */
	UFUNCTION(BlueprintCallable, Category = "Scriptable Framework|Graph")
	void CancelRunner(UScriptableGraphInstance* Runner);

	//~ USubsystem interface
	virtual void Deinitialize() override;
	//~ End of USubsystem interface

private:
	/** Adds a runner to the live set (idempotent). Called by the runner from Launch. */
	void RegisterRunner(UScriptableGraphInstance* Runner);

	/** Removes a runner from the live set. Called by the runner from Finish (and defensively on destroy). */
	void UnregisterRunner(UScriptableGraphInstance* Runner);

	/** Adds an action runner to the live set (idempotent). Called by the runner from Launch. */
	void RegisterActionRunner(UScriptableActionRunner* Runner);

	/** Removes an action runner from the live set. Called by the runner on finish (and defensively on destroy). */
	void UnregisterActionRunner(UScriptableActionRunner* Runner);

	/** World-teardown variant: cancels graph runners with CancelImmediate (skips Exit cleanup, the world
	 * and any actors it would touch are being destroyed) and action runners with Cancel. */
	void CancelAllForTeardown();

	/** Live graph runners. These strong refs keep runners alive while they execute. */
	UPROPERTY()
	TArray<TObjectPtr<UScriptableGraphInstance>> ActiveRunners;

	/** Live action runners (FScriptableAction executions); same role as ActiveRunners. */
	UPROPERTY()
	TArray<TObjectPtr<UScriptableActionRunner>> ActiveActionRunners;

	friend class UScriptableGraphInstance;
	friend class UScriptableActionRunner;
};

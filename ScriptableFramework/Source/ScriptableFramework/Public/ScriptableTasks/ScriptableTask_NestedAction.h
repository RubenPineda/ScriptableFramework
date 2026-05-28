// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableTasks/ScriptableTask.h"
#include "ScriptableTasks/ScriptableAction.h"
#include "ScriptableTask_NestedAction.generated.h"

/**
 * Wrapper task that runs an inline FScriptableAction. Useful for grouping a sub-flow so the task-level
 * Control (loop, doOnce, etc.) applies to the unit — e.g. "play dialogue + wait, looping". The inner
 * action inherits the parent scope's context: it does NOT declare its own.
 */
UCLASS(DisplayName = "Nested Action", meta = (TaskCategory = "System|Flow"))
class SCRIPTABLEFRAMEWORK_API UScriptableTask_NestedAction : public UScriptableTask
{
	GENERATED_BODY()

public:
	/** Optional descriptive label rendered in the row header (e.g. "Tutorial Steps"). */
	UPROPERTY(EditAnywhere, Category = "Nested Action", meta = (NoBinding))
	FString ActionName;

	/** The inner action (Tasks + Mode). Context is intentionally inherited from the parent scope. */
	UPROPERTY(EditAnywhere, Category = "Nested Action", meta = (ShowOnlyInnerProperties, NoBinding))
	FScriptableAction Action;

	//~ UScriptableObject interface
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	//~ End of UScriptableObject interface

protected:
	//~ UScriptableTask interface
	virtual void ResetTask() override;
	virtual void BeginTask() override;
	virtual void FinishTask() override;
	virtual void StopTask() override;
	//~ End of UScriptableTask interface

#if WITH_EDITOR
	virtual FText GetDisplayTitle() const override;
#endif

	/** Transient runtime copy of Action with its tasks cloned under `this` for lifecycle/state isolation. */
	UPROPERTY(Transient)
	FScriptableAction RuntimeAction;

private:
	/** Builds a fresh RuntimeAction by cloning the source Action's tasks under `this`, registers it,
	 * and subscribes to its OnActionFinish so the outer task completes when the inner action does. */
	void InstantiateRuntimeAction();

	/** Tears down the current RuntimeAction (unsubscribe + finish + unregister + drop tasks). */
	void TeardownRuntimeAction();

	/** Bound to RuntimeAction.OnActionFinish — completes the outer task. Task-level loop control then
	 * either re-enters BeginTask (loop) or propagates Finish to siblings. */
	void HandleActionFinished();
};

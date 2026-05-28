// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableTasks/ScriptableAction.h"
#include "ScriptableActionRunner.generated.h"

DECLARE_MULTICAST_DELEGATE(FScriptableActionRunnerFinishedNative);

/**
 * Runtime owner for a single FScriptableAction execution. Returned by the async "Run Scriptable
 * Action" node so callers can cancel the run, query its state, and subscribe to completion.
 *
 * Holds the action as a UPROPERTY so reflection roots its tasks here (otherwise they rely on their
 * original outers and can be GC'd at PIE end). Registered with UScriptableGraphSubsystem, which
 * keeps it alive while running and cancels it on world teardown.
 */
UCLASS(BlueprintType)
class SCRIPTABLEFRAMEWORK_API UScriptableActionRunner : public UObject
{
	GENERATED_BODY()

public:
	/** Moves the action into this runner and starts it. */
	void Launch(FScriptableAction&& InAction, UObject* InOwner);

	/** Cancels the run: force-finishes the action immediately. No-op if it already finished. */
	UFUNCTION(BlueprintCallable, Category = "Scriptable Framework|Action")
	void Cancel();

	/** Returns true while the action is executing. */
	UFUNCTION(BlueprintCallable, Category = "Scriptable Framework|Action")
	bool IsRunning() const { return Action.IsRunning(); }

	/** Fires once when the action finishes (naturally or via Cancel). */
	FScriptableActionRunnerFinishedNative OnFinishedNative;

	virtual void BeginDestroy() override;

private:
	void HandleActionFinished();

	/** The action lives here. UPROPERTY so reflection roots its Tasks via this runner — without it the
	 * tasks rely on their original outers and can be GC'd at PIE end while the subsystem still holds us. */
	UPROPERTY()
	FScriptableAction Action;

	/** Launch owner; world context used to resolve the subsystem for register/unregister. */
	UPROPERTY()
	TObjectPtr<UObject> Owner;

	friend class UScriptableGraphSubsystem;
};

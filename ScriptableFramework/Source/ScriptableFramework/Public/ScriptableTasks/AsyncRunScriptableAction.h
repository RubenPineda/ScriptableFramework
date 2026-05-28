// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "ScriptableTasks/ScriptableAction.h"
#include "ScriptableContext.h"
#include "AsyncRunScriptableAction.generated.h"

class UScriptableActionRunner;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAsyncScriptableActionEvent, UScriptableActionRunner*, Runner);

/** Async node that runs a FScriptableAction and exposes its live runner. */
UCLASS(MinimalAPI)
class UAsyncRunScriptableAction : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/**
	 * Runs a Scriptable Action. Started fires immediately with the live runner (use it to cancel
	 * or query the action while it runs); Finished fires when it completes, with the same runner.
	 */
	UFUNCTION(BlueprintCallable, Category = ScriptableAction, meta = (DefaultToSelf = "Owner", BlueprintInternalUseOnly = "true", DisplayName = "Run Scriptable Action"))
	static UAsyncRunScriptableAction* RunScriptableAction(UObject* Owner, FScriptableAction Action, const FScriptableContext& Context);

	/** Fired right after the action launches. Carries the live runner. */
	UPROPERTY(BlueprintAssignable)
	FAsyncScriptableActionEvent Started;

	/** Fired when the action finishes (naturally or cancelled). Carries the runner. */
	UPROPERTY(BlueprintAssignable)
	FAsyncScriptableActionEvent Finished;

	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;

private:
	void HandleActionFinished();

protected:
	UPROPERTY(Transient)
	TObjectPtr<UObject> ActionOwner;

	UPROPERTY(Transient)
	TObjectPtr<UScriptableActionRunner> Runner;

	/** Action moved into the runner at Activate. */
	UPROPERTY()
	FScriptableAction LaunchAction;

	/** Values seeded into the action's context bag at launch. */
	UPROPERTY()
	FScriptableContext LaunchContext;
};

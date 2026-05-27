// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableTasks/ScriptableAction.h"
#include "ScriptableActionRunner.generated.h"

/**
 * Runtime owner for fire-and-forget FScriptableAction executions. Internal — clients
 * use FScriptableAction::Run(MoveTemp(Action), Owner) and never see this type.
 *
 * Holds the action as a stable member (so AddRaw bindings inside the action stay valid). Registered
 * with UScriptableGraphSubsystem, which keeps it alive while running and cancels it on world teardown.
 */
UCLASS(Hidden)
class UScriptableActionRunner : public UObject
{
	GENERATED_BODY()

public:
	void Launch(FScriptableAction&& InAction, UObject* InOwner);

	virtual void BeginDestroy() override;

private:
	void HandleActionFinished();

	/** Force-finishes the action. Called by the subsystem on world teardown (before GC), where task Stop events are safe. */
	void CancelFromSubsystem();

	/** The action lives here. Stable address for the duration of this UObject. */
	FScriptableAction Action;

	/** Launch owner; world context used to resolve the subsystem for register/unregister. */
	UPROPERTY()
	TObjectPtr<UObject> Owner;

	friend class UScriptableGraphSubsystem;
};
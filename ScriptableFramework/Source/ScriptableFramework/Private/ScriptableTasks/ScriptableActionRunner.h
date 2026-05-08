// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableTasks/ScriptableAction.h"
#include "ScriptableActionRunner.generated.h"

/**
 * Runtime owner for fire-and-forget FScriptableAction executions. Internal — clients
 * use FScriptableAction::Run(MoveTemp(Action), Owner) and never see this type.
 *
 * Holds the action as a stable member (so AddRaw bindings inside the action stay
 * valid), and keeps itself alive via a self-reference until the action finishes.
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

	/** The action lives here. Stable address for the duration of this UObject. */
	FScriptableAction Action;

	/** Self-reference keeps the GC from collecting us while the action runs. */
	UPROPERTY()
	TObjectPtr<UScriptableActionRunner> SelfReference;
};
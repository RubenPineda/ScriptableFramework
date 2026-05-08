// Copyright 2026 kirzo

#include "ScriptableActionRunner.h"

void UScriptableActionRunner::Launch(FScriptableAction&& InAction, UObject* InOwner)
{
	Action = MoveTemp(InAction);

	// Anchor ourselves so the GC can't collect us mid-run.
	SelfReference = this;

	// Subscribe BEFORE Run, in case the action finishes synchronously (empty Tasks list).
	Action.OnActionFinish.AddUObject(this, &UScriptableActionRunner::HandleActionFinished);

	Action.Run(InOwner);
}

void UScriptableActionRunner::HandleActionFinished()
{
	// Drop the anchor; next GC sweep will collect us.
	// Cleanup of the action's own state happened inside Finish() already.
	SelfReference = nullptr;
}

void UScriptableActionRunner::BeginDestroy()
{
	// If the runner is being destroyed while still running (owner died, world tearing
	// down, etc.), force-finish so child tasks unregister cleanly.
	if (Action.IsRunning())
	{
		Action.Finish(/*bForce=*/true);
	}
	Super::BeginDestroy();
}
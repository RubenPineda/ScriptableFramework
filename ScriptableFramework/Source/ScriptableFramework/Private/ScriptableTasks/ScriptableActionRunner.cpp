// Copyright 2026 kirzo

#include "ScriptableActionRunner.h"
#include "ScriptableNodes/ScriptableGraphSubsystem.h"

void UScriptableActionRunner::Launch(FScriptableAction&& InAction, UObject* InOwner)
{
	Action = MoveTemp(InAction);
	Owner = InOwner;

	// Register with the subsystem: it keeps us alive while running and cancels us on world teardown
	// (before GC). No subsystem means no world (CDO/transient) -> nothing to run against.
	UScriptableGraphSubsystem* Subsystem = UScriptableGraphSubsystem::Get(InOwner);
	if (!Subsystem) return;
	Subsystem->RegisterActionRunner(this);

	// Subscribe BEFORE Run, in case the action finishes synchronously (empty Tasks list).
	Action.OnActionFinish.AddUObject(this, &UScriptableActionRunner::HandleActionFinished);

	Action.Run(InOwner);
}

void UScriptableActionRunner::HandleActionFinished()
{
	// Unregister; once the subsystem drops its strong ref we become collectible. The action's own
	// cleanup happened inside Finish() already.
	if (UScriptableGraphSubsystem* Subsystem = UScriptableGraphSubsystem::Get(Owner))
	{
		Subsystem->UnregisterActionRunner(this);
	}
}

void UScriptableActionRunner::CancelFromSubsystem()
{
	// Called from world teardown, before GC — safe to fire task Stop events. The finish callback
	// (HandleActionFinished) unregisters us.
	if (Action.IsRunning())
	{
		Action.Finish(/*bForce=*/true);
	}
}

void UScriptableActionRunner::BeginDestroy()
{
	// Defensive: drop ourselves from the subsystem if we reach GC still registered. We deliberately do
	// NOT finish the action here — firing task Stop events during GC is illegal. World teardown already
	// cancelled us via the subsystem (Deinitialize) in a safe context before GC ran.
	if (UScriptableGraphSubsystem* Subsystem = UScriptableGraphSubsystem::Get(Owner))
	{
		Subsystem->UnregisterActionRunner(this);
	}
	Super::BeginDestroy();
}
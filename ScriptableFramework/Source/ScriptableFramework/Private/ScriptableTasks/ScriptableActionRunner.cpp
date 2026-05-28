// Copyright 2026 kirzo

#include "ScriptableTasks/ScriptableActionRunner.h"
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
	// Notify external subscribers (e.g. the async BP node) before we detach from the subsystem.
	OnFinishedNative.Broadcast();

	// Unregister; once the subsystem drops its strong ref we become collectible. The action's own
	// cleanup happened inside Finish() already.
	if (UScriptableGraphSubsystem* Subsystem = UScriptableGraphSubsystem::Get(Owner))
	{
		Subsystem->UnregisterActionRunner(this);
	}
}

void UScriptableActionRunner::Cancel()
{
	// Force-finish the action. OnActionFinish -> HandleActionFinished unregisters us. Used both by
	// user code (manual cancel) and by the subsystem on world teardown — task Stop events are safe
	// in either context (world teardown calls this before GC).
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
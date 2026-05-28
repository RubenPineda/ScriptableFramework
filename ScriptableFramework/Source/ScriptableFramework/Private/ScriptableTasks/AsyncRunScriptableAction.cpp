// Copyright 2026 kirzo

#include "ScriptableTasks/AsyncRunScriptableAction.h"
#include "ScriptableTasks/ScriptableActionRunner.h"
#include "ScriptableTasks/ScriptableTask.h"

UAsyncRunScriptableAction* UAsyncRunScriptableAction::RunScriptableAction(UObject* Owner, FScriptableAction& Action, const FScriptableContext& Context)
{
	UAsyncRunScriptableAction* Node = NewObject<UAsyncRunScriptableAction>(Owner);

	Node->ActionOwner = Owner;
	Node->SourceAction = &Action;
	Node->LaunchContext = Context;

	if (Owner)
	{
		Node->RegisterWithGameInstance(Owner);
	}

	return Node;
}

void UAsyncRunScriptableAction::Activate()
{
	Super::Activate();

	if (!ActionOwner || !SourceAction)
	{
		SetReadyToDestroy();
		return;
	}

	// Spawn the runner and deep-copy the source action's tasks into it. Clone owns the new tasks via
	// the Runner so the source BP variable is never mutated and concurrent runs don't share state.
	Runner = NewObject<UScriptableActionRunner>(ActionOwner);
	if (!Runner)
	{
		SetReadyToDestroy();
		return;
	}

	FScriptableAction ClonedAction = SourceAction->Clone(Runner);
	ClonedAction.SetContext(LaunchContext);
	Runner->Launch(MoveTemp(ClonedAction), ActionOwner);

	// Source pointer is consumed; the BP variable's lifetime is not ours.
	SourceAction = nullptr;

	// Hand the runner out immediately so callers can drive it (Cancel, IsRunning) while it runs.
	Started.Broadcast(Runner);

	// An empty-Tasks action finishes synchronously inside Launch, before we could subscribe.
	if (!Runner->IsRunning())
	{
		HandleActionFinished();
		return;
	}

	Runner->OnFinishedNative.AddUObject(this, &UAsyncRunScriptableAction::HandleActionFinished);
}

void UAsyncRunScriptableAction::HandleActionFinished()
{
	if (Runner)
	{
		Runner->OnFinishedNative.RemoveAll(this);
	}

	Finished.Broadcast(Runner);
	SetReadyToDestroy();
}

void UAsyncRunScriptableAction::SetReadyToDestroy()
{
	if (Runner)
	{
		Runner->OnFinishedNative.RemoveAll(this);
	}

	Runner = nullptr;
	ActionOwner = nullptr;
	SourceAction = nullptr;

	Super::SetReadyToDestroy();
}

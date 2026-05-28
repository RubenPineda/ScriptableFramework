// Copyright 2026 kirzo

#include "ScriptableTasks/AsyncRunScriptableAction.h"
#include "ScriptableTasks/ScriptableActionRunner.h"
#include "ScriptableTasks/ScriptableTask.h"

UAsyncRunScriptableAction* UAsyncRunScriptableAction::RunScriptableAction(UObject* Owner, FScriptableAction Action, const FScriptableContext& Context)
{
	UAsyncRunScriptableAction* Node = NewObject<UAsyncRunScriptableAction>(Owner);

	Node->ActionOwner = Owner;
	Node->LaunchAction = MoveTemp(Action);
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

	if (!ActionOwner)
	{
		SetReadyToDestroy();
		return;
	}

	// FScriptableAction::Run creates a UScriptableActionRunner that owns the action and its tasks,
	// applies the context, and starts execution. The runner survives until the action finishes.
	Runner = FScriptableAction::Run(MoveTemp(LaunchAction), ActionOwner, LaunchContext);
	if (!Runner)
	{
		SetReadyToDestroy();
		return;
	}

	// Hand the runner out immediately so callers can drive it (Cancel, IsRunning) while it runs.
	Started.Broadcast(Runner);

	// An empty-Tasks action finishes synchronously inside Run, before we could subscribe.
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

	Super::SetReadyToDestroy();
}

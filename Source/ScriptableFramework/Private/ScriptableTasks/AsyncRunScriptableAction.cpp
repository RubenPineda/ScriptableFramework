// Copyright 2026 kirzo

#include "ScriptableTasks/AsyncRunScriptableAction.h"
#include "ScriptableTasks/ScriptableTask.h"

UAsyncRunScriptableAction* UAsyncRunScriptableAction::RunScriptableAction(UObject* Owner, FScriptableAction& Action, bool bReset)
{
	UAsyncRunScriptableAction* Node = NewObject<UAsyncRunScriptableAction>(Owner);

	Node->ActionOwner = Owner;
	Node->TargetAction = &Action;
	Node->bShouldReset = bReset;

	if (Owner)
	{
		Node->RegisterWithGameInstance(Owner);
	}

	return Node;
}

void UAsyncRunScriptableAction::Activate()
{
	Super::Activate();

	if (!ActionOwner || !TargetAction)
	{
		SetReadyToDestroy();
		return;
	}

	if (bShouldReset)
	{
		TargetAction->Reset();
	}

	TargetAction->OnActionFinish.RemoveAll(this);
	TargetAction->OnActionFinish.AddUObject(this, &UAsyncRunScriptableAction::HandleActionFinished);

	TargetAction->Register(ActionOwner);
	TargetAction->Begin();
}

void UAsyncRunScriptableAction::HandleActionFinished()
{
	if (TargetAction)
	{
		TargetAction->OnActionFinish.RemoveAll(this);
		TargetAction->Unregister();
	}

	OnFinish.Broadcast();
	SetReadyToDestroy();
}

void UAsyncRunScriptableAction::SetReadyToDestroy()
{
	TargetAction = nullptr;
	ActionOwner = nullptr;

	Super::SetReadyToDestroy();
}
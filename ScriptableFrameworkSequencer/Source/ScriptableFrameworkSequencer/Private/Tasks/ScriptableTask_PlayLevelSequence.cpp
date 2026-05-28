// Copyright 2026 kirzo

#include "Tasks/ScriptableTask_PlayLevelSequence.h"
#include "LevelSequencePlayer.h"

void UScriptableTask_PlayLevelSequence::BeginTask()
{
	if (!Player)
	{
		Finish();
		return;
	}

	if (bWaitUntilFinished)
	{
		// Bind before Play so we don't miss an instantaneous OnFinished.
		Player->OnFinished.AddDynamic(this, &UScriptableTask_PlayLevelSequence::HandleSequenceFinished);
		Player->Play();
		// Finish() is called from HandleSequenceFinished.
	}
	else
	{
		Player->Play();
		Finish();
	}
}

void UScriptableTask_PlayLevelSequence::FinishTask()
{
	// Defensive cleanup: covers both natural completion (binding already
	// removed in the handler is a no-op) and external Finish() (e.g. action
	// cancelled mid-play, where the binding would otherwise leak).
	if (Player)
	{
		Player->OnFinished.RemoveDynamic(this, &UScriptableTask_PlayLevelSequence::HandleSequenceFinished);
	}
}

void UScriptableTask_PlayLevelSequence::StopTask()
{
	if (Player)
	{
		// Unbind first so the Stop-triggered OnFinished can't try to Finish() this already-stopped task.
		Player->OnFinished.RemoveDynamic(this, &UScriptableTask_PlayLevelSequence::HandleSequenceFinished);
		Player->Stop();
	}
}

void UScriptableTask_PlayLevelSequence::HandleSequenceFinished()
{
	Finish();
}

#if WITH_EDITOR
FText UScriptableTask_PlayLevelSequence::GetDisplayTitle() const
{
	FString PlayerName;
	if (!GetBindingDisplayText(GET_MEMBER_NAME_CHECKED(UScriptableTask_PlayLevelSequence, Player), PlayerName))
	{
		PlayerName = TEXT("None");
	}

	const FText Suffix = bWaitUntilFinished ? INVTEXT(" and wait") : FText::GetEmpty();
	return FText::Format(INVTEXT("Play {0}{1}"), FText::FromString(PlayerName), Suffix);
}
#endif
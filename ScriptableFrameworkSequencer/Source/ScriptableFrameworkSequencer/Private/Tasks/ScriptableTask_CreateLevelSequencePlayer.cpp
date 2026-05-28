// Copyright 2026 kirzo

#include "Tasks/ScriptableTask_CreateLevelSequencePlayer.h"
#include "LevelSequence.h"
#include "LevelSequencePlayer.h"
#include "LevelSequenceActor.h"
#include "Engine/World.h"

void UScriptableTask_CreateLevelSequencePlayer::BeginTask()
{
	// Synchronous: creating a player is instant.
	if (!LevelSequence)
	{
		Finish();
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		Finish();
		return;
	}

	ALevelSequenceActor* OutActor = nullptr;
	Player = ULevelSequencePlayer::CreateLevelSequencePlayer(World, LevelSequence, PlaybackSettings, OutActor);
	LevelSequenceActor = OutActor;

	if (!bPlayAutomatically)
	{
		// Player created but not auto-playing: nothing more to do.
		Finish();
		return;
	}

	if (bWaitUntilFinished)
	{
		// Bind before Play so we don't miss an instantaneous OnFinished.
		Player->OnFinished.AddDynamic(this, &UScriptableTask_CreateLevelSequencePlayer::HandleSequenceFinished);
		Player->Play();
		// Finish() is called from HandleSequenceFinished.
	}
	else
	{
		Player->Play();
		Finish();
	}
}

void UScriptableTask_CreateLevelSequencePlayer::FinishTask()
{
	// Defensive cleanup: covers both natural completion (binding already
	// removed in the handler is a no-op) and external Finish() (e.g. action
	// cancelled mid-play, where the binding would otherwise leak).
	if (Player)
	{
		Player->OnFinished.RemoveDynamic(this, &UScriptableTask_CreateLevelSequencePlayer::HandleSequenceFinished);
	}
}

void UScriptableTask_CreateLevelSequencePlayer::StopTask()
{
	// Cancel mid-play: unbind first so a Stop-triggered OnFinished can't try to Finish() this
	// already-stopped task, then halt the player. ResetTask will destroy the actor on re-run.
	if (Player)
	{
		Player->OnFinished.RemoveDynamic(this, &UScriptableTask_CreateLevelSequencePlayer::HandleSequenceFinished);
		if (Player->IsPlaying())
		{
			Player->Stop();
		}
	}
}

void UScriptableTask_CreateLevelSequencePlayer::HandleSequenceFinished()
{
	Finish();
}

void UScriptableTask_CreateLevelSequencePlayer::ResetTask()
{
	// Tear down the spawned actor + player so loops / re-runs don't leak.
	// CreateLevelSequencePlayer spawns the LevelSequenceActor through the world; Destroy() takes care of the player too,
	// since it's owned by the actor.
	if (LevelSequenceActor)
	{
		if (Player && Player->IsPlaying())
		{
			Player->Stop();
		}
		LevelSequenceActor->Destroy();
	}

	Player = nullptr;
	LevelSequenceActor = nullptr;
}

#if WITH_EDITOR
FText UScriptableTask_CreateLevelSequencePlayer::GetDisplayTitle() const
{
	FString SequenceName;
	if (!GetBindingDisplayText(GET_MEMBER_NAME_CHECKED(UScriptableTask_CreateLevelSequencePlayer, LevelSequence), SequenceName))
	{
		SequenceName = LevelSequence ? LevelSequence->GetName() : TEXT("None");
	}

	FText Suffix = FText::GetEmpty();
	if (bPlayAutomatically)
	{
		Suffix = bWaitUntilFinished ? INVTEXT(", play and wait") : INVTEXT(" and play");
	}
	return FText::Format(INVTEXT("Create Player [{0}]{1}"), FText::FromString(SequenceName), Suffix);
}
#endif
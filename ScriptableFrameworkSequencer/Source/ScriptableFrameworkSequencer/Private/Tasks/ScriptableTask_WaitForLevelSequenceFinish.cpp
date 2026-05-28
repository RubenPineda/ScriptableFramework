// Copyright 2026 kirzo

#include "Tasks/ScriptableTask_WaitForLevelSequenceFinish.h"
#include "LevelSequencePlayer.h"

void UScriptableTask_WaitForLevelSequenceFinish::BeginTask()
{
	if (!Player || !Player->IsPlaying())
	{
		Finish();
		return;
	}

	Player->OnFinished.AddDynamic(this, &UScriptableTask_WaitForLevelSequenceFinish::HandleSequenceFinished);
}

void UScriptableTask_WaitForLevelSequenceFinish::FinishTask()
{
	if (Player)
	{
		Player->OnFinished.RemoveDynamic(this, &UScriptableTask_WaitForLevelSequenceFinish::HandleSequenceFinished);
	}
}

void UScriptableTask_WaitForLevelSequenceFinish::StopTask()
{
	// Cancel mid-wait: just stop listening. This task doesn't own the player, so we don't touch playback.
	if (Player)
	{
		Player->OnFinished.RemoveDynamic(this, &UScriptableTask_WaitForLevelSequenceFinish::HandleSequenceFinished);
	}
}

void UScriptableTask_WaitForLevelSequenceFinish::HandleSequenceFinished()
{
	Finish();
}

#if WITH_EDITOR
FText UScriptableTask_WaitForLevelSequenceFinish::GetDisplayTitle() const
{
	FString PlayerName;
	if (!GetBindingDisplayText(GET_MEMBER_NAME_CHECKED(UScriptableTask_WaitForLevelSequenceFinish, Player), PlayerName))
	{
		PlayerName = TEXT("None");
	}

	return FText::Format(INVTEXT("Wait For {0} To Finish"), FText::FromString(PlayerName));
}
#endif
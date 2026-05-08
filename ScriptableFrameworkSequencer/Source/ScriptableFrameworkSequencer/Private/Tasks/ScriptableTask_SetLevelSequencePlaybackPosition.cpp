// Copyright 2026 kirzo

#include "Tasks/ScriptableTask_SetLevelSequencePlaybackPosition.h"
#include "LevelSequencePlayer.h"

void UScriptableTask_SetLevelSequencePlaybackPosition::BeginTask()
{
	if (Player)
	{
		Player->SetPlaybackPosition(PlaybackParams);
	}

	// Synchronous: the position change is instant. Finish so the action can advance.
	Finish();
}

#if WITH_EDITOR
FText UScriptableTask_SetLevelSequencePlaybackPosition::GetDisplayTitle() const
{
	FString PlayerName;
	if (!GetBindingDisplayText(GET_MEMBER_NAME_CHECKED(UScriptableTask_SetLevelSequencePlaybackPosition, Player), PlayerName))
	{
		PlayerName = TEXT("None");
	}

	return FText::Format(INVTEXT("Set Playback Position on {0}"), FText::FromString(PlayerName));
}
#endif
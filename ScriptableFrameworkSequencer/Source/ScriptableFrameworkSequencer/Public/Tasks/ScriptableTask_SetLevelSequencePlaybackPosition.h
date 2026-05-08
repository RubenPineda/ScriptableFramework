// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableTasks/ScriptableTask.h"
#include "MovieSceneSequencePlayer.h"
#include "ScriptableTask_SetLevelSequencePlaybackPosition.generated.h"

class ULevelSequencePlayer;

/** Sets the playback position of a Level Sequence Player. */
UCLASS(DisplayName = "Set Level Sequence Playback Position", meta = (TaskCategory = "LevelSequence"))
class UScriptableTask_SetLevelSequencePlaybackPosition : public UScriptableTask
{
	GENERATED_BODY()

public:
	/** The sequence player whose playback position will be set. */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<ULevelSequencePlayer> Player = nullptr;

	/** Playback position parameters (frame, evaluation type, jump behavior, etc.). */
	UPROPERTY(EditAnywhere, Category = "Config", meta = (ShowOnlyInnerProperties))
	FMovieSceneSequencePlaybackParams PlaybackParams;

protected:
	virtual void BeginTask() override;

#if WITH_EDITOR
	virtual FText GetDisplayTitle() const override;
#endif
};
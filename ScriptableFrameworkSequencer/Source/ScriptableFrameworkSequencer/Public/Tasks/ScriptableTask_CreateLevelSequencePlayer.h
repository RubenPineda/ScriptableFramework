// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableTasks/ScriptableTask.h"
#include "MovieSceneSequencePlayer.h"
#include "ScriptableTask_CreateLevelSequencePlayer.generated.h"

class ULevelSequence;
class ULevelSequencePlayer;
class ALevelSequenceActor;

/**
 * Creates a Level Sequence Player at runtime, mirroring the "Create Level Sequence Player" Blueprint node.
 *
 * Exposes the resulting Player as an Output so subsequent tasks can bind to it via Sibling bindings.
 *
 * If bPlayAutomatically is true the player starts playing immediately after creation; otherwise the caller
 * is expected to drive playback through one of the other Sequencer tasks.
 *
 * Reset() destroys the spawned LevelSequenceActor so loops/re-runs don't leak players in the world.
 */
UCLASS(DisplayName = "Create Level Sequence Player", meta = (TaskCategory = "LevelSequence"))
class UScriptableTask_CreateLevelSequencePlayer : public UScriptableTask
{
	GENERATED_BODY()

public:
	/** The Level Sequence asset to play. */
	UPROPERTY(EditAnywhere, Category = "Config")
	TObjectPtr<ULevelSequence> LevelSequence = nullptr;

	/** Playback settings (loop count, play rate, restore state, …). */
	UPROPERTY(EditAnywhere, Category = "Config")
	FMovieSceneSequencePlaybackSettings PlaybackSettings;

	/** If true, the newly created player starts playing immediately. Otherwise it stays idle until another task drives it. */
	UPROPERTY(EditAnywhere, Category = "Config")
	bool bPlayAutomatically = false;

	/** If true, the task waits for OnFinished before completing. If false, it finishes immediately after kicking off Play. */
	UPROPERTY(EditAnywhere, Category = "Config", meta = (EditCondition = "bPlayAutomatically"))
	bool bWaitUntilFinished = true;

	/** The created Level Sequence Player. Bind subsequent Sequencer tasks (Play / Wait / SetPosition) to this. */
	UPROPERTY(VisibleAnywhere, Transient, BlueprintReadOnly, Category = "Output")
	TObjectPtr<ULevelSequencePlayer> Player = nullptr;

	/** The Level Sequence Actor spawned to host the player. Bind to this if you need to manipulate it (Destroy, attach, …). */
	UPROPERTY(VisibleAnywhere, Transient, BlueprintReadOnly, Category = "Output")
	TObjectPtr<ALevelSequenceActor> LevelSequenceActor = nullptr;

protected:
	virtual void BeginTask() override;
	virtual void FinishTask() override;
	virtual void StopTask() override;
	virtual void ResetTask() override;

#if WITH_EDITOR
	virtual FText GetDisplayTitle() const override;
#endif

private:
	UFUNCTION()
	void HandleSequenceFinished();
};
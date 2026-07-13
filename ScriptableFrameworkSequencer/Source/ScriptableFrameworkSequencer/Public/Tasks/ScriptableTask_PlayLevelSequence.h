// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableTasks/ScriptableTask.h"
#include "ScriptableTask_PlayLevelSequence.generated.h"

class ULevelSequencePlayer;

/** Plays a Level Sequence Player, optionally waiting until the sequence finishes before completing. */
UCLASS(DisplayName = "Play Level Sequence", meta = (TaskCategory = "LevelSequence"))
class UScriptableTask_PlayLevelSequence : public UScriptableTask
{
	GENERATED_BODY()

public:
	/** The sequence player to play. */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<ULevelSequencePlayer> Player = nullptr;

	/** If true, the task waits for OnFinished before completing. If false, it finishes immediately after kicking off Play. */
	UPROPERTY(EditAnywhere, Category = "Config")
	bool bWaitUntilFinished = true;

	/** If set, stopping the task mid-play jumps the sequence to its last frame (GoToEndAndStop) instead of freezing where it is. Only reachable while waiting (otherwise the task has already finished). */
	UPROPERTY(EditAnywhere, Category = "Config", meta = (EditCondition = "bWaitUntilFinished"))
	bool bGoToEndOnCancel = false;

protected:
	virtual void BeginTask() override;
	virtual void FinishTask() override;
	virtual void StopTask() override;

#if WITH_EDITOR
	virtual FText GetDisplayTitle() const override;
#endif

private:
	UFUNCTION()
	void HandleSequenceFinished();
};
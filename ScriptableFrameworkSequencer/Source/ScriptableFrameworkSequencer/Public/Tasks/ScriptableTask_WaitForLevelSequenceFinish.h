// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableTasks/ScriptableTask.h"
#include "ScriptableTask_WaitForLevelSequenceFinish.generated.h"

class ULevelSequencePlayer;

/** Waits for a Level Sequence Player's OnFinished event before completing. Useful when Play was started elsewhere. */
UCLASS(DisplayName = "Wait For Level Sequence Finish", meta = (TaskCategory = "LevelSequence"))
class UScriptableTask_WaitForLevelSequenceFinish : public UScriptableTask
{
	GENERATED_BODY()

public:
	/** The sequence player to wait on. */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<ULevelSequencePlayer> Player = nullptr;

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
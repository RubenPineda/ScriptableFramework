// Copyright 2026 kirzo

#include "ScriptableTasks/ScriptableTask.h"

UE_DISABLE_OPTIMIZATION

DEFINE_LOG_CATEGORY(LogScriptableTask);

void UScriptableTask::OnUnregister()
{
	Super::OnUnregister();

	OnTaskBeginNative.Clear();
	OnTaskFinishNative.Clear();
	OnTaskBegin.Clear();
	OnTaskFinish.Clear();
}

void UScriptableTask::Reset()
{
	if (HasFinished())
	{
		Status = EScriptableTaskStatus::None;
		CurrentLoopIndex = 0;
		bDoOnceFinished = false;
		ResetTask();
	}
}

void UScriptableTask::Begin()
{
	check(bRegistered);

	if (Control.bDoOnce && bDoOnceFinished)
	{
		// We treat it as if it started and immediately finished successfully.
		// This ensures the Action sequence proceeds to the next task.
		OnTaskFinishNative.Broadcast(this);
		OnTaskFinish.Broadcast(this);
		return;
	}

	check(Status != EScriptableTaskStatus::Begun);

	CurrentLoopIndex = 0;

	ResolveBindings();

	Status = EScriptableTaskStatus::Begun;
	RegisterTickFunctions(true);
	BeginTask();

	OnTaskBeginNative.Broadcast(this);
	OnTaskBegin.Broadcast(this);
}

void UScriptableTask::Finish()
{
	if (HasBegun() && !HasFinished() && IsEnabled())
	{
		if (Control.bLoop)
		{
			CurrentLoopIndex++;

			// 0 means Infinite, otherwise check strictly against count
			if (Control.LoopCount <= 0 || CurrentLoopIndex < Control.LoopCount)
			{
				// Restart the task logic without changing Status or broadcasting Finish.
				// Note: We don't call Begin() to avoid resetting CurrentLoopIndex.
				// We call the virtual implementation directly.
				BeginTask();
				return; // Task is NOT finished yet.
			}
		}

		// Mark as finished for future runs.
		if (Control.bDoOnce)
		{
			bDoOnceFinished = true;
		}

		Status = EScriptableTaskStatus::Finished;
		RegisterTickFunctions(false);
		FinishTask();

		OnTaskFinishNative.Broadcast(this);
		OnTaskFinish.Broadcast(this);
	}
}

void UScriptableTask::ResetTask()
{
	ReceiveResetTask();
}

void UScriptableTask::BeginTask()
{
	ReceiveBeginTask();
}

void UScriptableTask::FinishTask()
{
	ReceiveFinishTask();
}

UE_ENABLE_OPTIMIZATION
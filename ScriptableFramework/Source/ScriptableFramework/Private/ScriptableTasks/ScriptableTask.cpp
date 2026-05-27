// Copyright 2026 kirzo

#include "ScriptableTasks/ScriptableTask.h"

DEFINE_LOG_CATEGORY(LogScriptableTask);

const FName UScriptableTask::CompletedOutputName = TEXT("Completed");
const FName UScriptableTask::StoppedOutputName = TEXT("Stopped");

TArray<FName> UScriptableTask::GetOutputPins() const
{
	// Completed is always present; append the author-defined extras (skip None, dedupe).
	TArray<FName> Outputs = { CompletedOutputName };
	for (const FName& Output : CustomOutputs)
	{
		if (!Output.IsNone())
		{
			Outputs.AddUnique(Output);
		}
	}
	return Outputs;
}

void UScriptableTask::OnUnregister()
{
	Super::OnUnregister();

	OnTaskBeginNative.Clear();
	OnTaskFinishNative.Clear();
	OnTaskStoppedNative.Clear();
	OnTaskBegin.Clear();
	OnTaskFinish.Clear();
	OnTaskStopped.Clear();
}

void UScriptableTask::Reset()
{
	if (HasFinished() || HasStopped())
	{
		Status = EScriptableTaskStatus::None;
		CurrentLoopIndex = 0;
		LastFiredOutput = NAME_None;
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
		LastFiredOutput = CompletedOutputName;
		OnTaskFinishNative.Broadcast(this);
		OnTaskFinish.Broadcast(this);
		return;
	}

	check(Status != EScriptableTaskStatus::Begun);

	CurrentLoopIndex = 0;
	LastFiredOutput = NAME_None;

	ResolveBindings();

	Status = EScriptableTaskStatus::Begun;
	RegisterTickFunctions(true);
	BeginTask();

	OnTaskBeginNative.Broadcast(this);
	OnTaskBegin.Broadcast(this);
}

void UScriptableTask::Finish()
{
	FinishWithOutput(CompletedOutputName);
}

void UScriptableTask::FinishWithOutput(FName OutputName)
{
	if (HasBegun() && !HasFinished() && !HasStopped() && IsEnabled())
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
		LastFiredOutput = OutputName;
		RegisterTickFunctions(false);
		FinishTask();

		OnTaskFinishNative.Broadcast(this);
		OnTaskFinish.Broadcast(this);
	}
}

void UScriptableTask::Stop()
{
	if (!IsStoppable()) return;
	if (!HasBegun() || HasFinished() || HasStopped()) return;
	if (!IsEnabled()) return;

	// Stop interrupts looping outright and does not mark bDoOnceFinished.
	Status = EScriptableTaskStatus::Stopped;
	LastFiredOutput = StoppedOutputName;
	RegisterTickFunctions(false);
	StopTask();

	OnTaskStoppedNative.Broadcast(this);
	OnTaskStopped.Broadcast(this);
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

void UScriptableTask::StopTask()
{
	ReceiveStopTask();
}
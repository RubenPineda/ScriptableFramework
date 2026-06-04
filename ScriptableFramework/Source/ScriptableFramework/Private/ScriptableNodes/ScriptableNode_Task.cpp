// Copyright 2026 kirzo

#include "ScriptableNodes/ScriptableNode_Task.h"
#include "ScriptableTasks/ScriptableTask.h"

const FName UScriptableNode_Task::StartInputName = TEXT("Start");

TArray<FName> UScriptableNode_Task::GetInputPins() const
{
	TArray<FName> Inputs = { StartInputName };
	if (Task && Task->IsStoppable())
	{
		Inputs.Add(UScriptableNode::StopInputName);
	}
	return Inputs;
}

TArray<FName> UScriptableNode_Task::GetDeclaredOutputPins() const
{
	return Task ? Task->GetOutputPins() : TArray<FName>{};
}

UScriptableObject* UScriptableNode_Task::GetBindingProxy() const
{
	return Task;
}

FString UScriptableNode_Task::GetTraceLabel() const
{
	if (!Task) return TEXT("Task(empty)");
	FString TaskClass = Task->GetClass()->GetName();
	TaskClass.RemoveFromStart(TEXT("ScriptableTask_"));
	return FString::Printf(TEXT("Task(%s)"), *TaskClass);
}

void UScriptableNode_Task::OnRegister()
{
	Super::OnRegister();

	if (Task)
	{
		// Propagate context/binding data into the inner task so it can resolve its own bindings.
		PropagateRuntimeData(Task);

		if (Task->IsEnabled())
		{
			Task->Register(GetOwner());
		}
	}
}

void UScriptableNode_Task::OnUnregister()
{
	if (Task)
	{
		Task->OnTaskFinishNative.RemoveAll(this);
		Task->OnTaskStoppedNative.RemoveAll(this);

		if (Task->IsEnabled())
		{
			Task->Unregister();
		}
	}

	Super::OnUnregister();
}

void UScriptableNode_Task::ProcessInput(FName InputName)
{
	if (InputName == StartInputName)
	{
		MarkInputInactive(StartInputName);
		if (!Task) return;

		// Arm every output the task might fire. Outputs not fired will be deactivated on finish/stop.
		for (const FName& OutputName : GetOutputPins())
		{
			MarkOutputActive(OutputName);
		}

		// Subscribe before begin in case the task finishes synchronously inside Begin().
		Task->OnTaskFinishNative.AddUObject(this, &UScriptableNode_Task::HandleTaskFinished);
		Task->OnTaskStoppedNative.AddUObject(this, &UScriptableNode_Task::HandleTaskStopped);

		Task->Begin();
	}
	else if (InputName == UScriptableNode::StopInputName)
	{
		MarkInputInactive(UScriptableNode::StopInputName);
		if (!Task) return;

		// Stop() will trigger OnTaskStoppedNative, which routes through HandleTaskStopped below.
		Task->Stop();
	}
}

void UScriptableNode_Task::Teardown()
{
	if (!Task) return;

	// Unsubscribe first so the task's cancellation does not propagate to a Stopped pin fire.
	Task->OnTaskFinishNative.RemoveAll(this);
	Task->OnTaskStoppedNative.RemoveAll(this);

	if (Task->IsStoppable() && Task->HasBegun() && !Task->HasFinished() && !Task->HasStopped())
	{
		Task->Stop();
	}

	DeactivateAllOutputs();
}

void UScriptableNode_Task::HandleTaskFinished(UScriptableTask* InTask)
{
	if (!InTask || InTask != Task) return;

	Task->OnTaskFinishNative.RemoveAll(this);
	Task->OnTaskStoppedNative.RemoveAll(this);

	const FName FiredOutput = InTask->GetLastFiredOutput();

	// Cancel any outputs we had armed but did not fire.
	for (const FName& OutputName : GetOutputPins())
	{
		if (OutputName != FiredOutput)
		{
			MarkOutputInactive(OutputName);
		}
	}

	FireOutput(FiredOutput);
}

void UScriptableNode_Task::HandleTaskStopped(UScriptableTask* InTask)
{
	if (!InTask || InTask != Task) return;

	Task->OnTaskFinishNative.RemoveAll(this);
	Task->OnTaskStoppedNative.RemoveAll(this);

	// Cancel every normal output; only Stopped will fire.
	for (const FName& OutputName : GetOutputPins())
	{
		if (OutputName != UScriptableNode::StoppedOutputName)
		{
			MarkOutputInactive(OutputName);
		}
	}

	FireOutput(UScriptableNode::StoppedOutputName);
}
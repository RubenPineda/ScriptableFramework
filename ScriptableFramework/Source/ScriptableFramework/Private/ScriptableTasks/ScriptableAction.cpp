// Copyright 2026 kirzo

#include "ScriptableTasks/ScriptableAction.h"
#include "ScriptableTasks/ScriptableTask.h"
#include "ScriptableTasks/ScriptableActionRunner.h"
#include "ScriptableContext.h"

FScriptableAction::FScriptableAction()
{
}

FScriptableAction::~FScriptableAction()
{
}

FScriptableAction FScriptableAction::Clone(UObject* NewOuter) const
{
	// 1. Shallow copy of the base properties (Context values, Mode, etc.)
	FScriptableAction ClonedAction = *this;

	// 2. Clear runtime state so the copy starts fresh
	ClonedAction.bIsRunning = false;
	ClonedAction.CurrentTaskIndex = 0;
	ClonedAction.OnActionBegin.Clear();
	ClonedAction.OnActionFinish.Clear();

	// Reconstruct context. Otherwise, the clone might
	// share memory pointers with the original template.
	ClonedAction.ConstructContext();

	// 3. Deep copy the Tasks array to avoid mutating the Data Asset
	ClonedAction.Tasks.Empty(Tasks.Num());

	for (UScriptableTask* Task : Tasks)
	{
		if (Task)
		{
			// DuplicateObject creates a real memory copy with NewOuter as the owner
			UScriptableTask* ClonedTask = DuplicateObject<UScriptableTask>(Task, NewOuter);
			ClonedAction.Tasks.Add(ClonedTask);
		}
	}

	return ClonedAction;
}

void FScriptableAction::Run(UObject* InOwner)
{
	if (!InOwner) return;

	if (IsRunning())
	{
		Finish();
	}

	Register(InOwner);
	Begin();
}

UScriptableActionRunner* FScriptableAction::Run(FScriptableAction&& Action, UObject* Owner)
{
	if (!Owner) return nullptr;

	UScriptableActionRunner* Runner = NewObject<UScriptableActionRunner>(Owner);
	Runner->Launch(MoveTemp(Action), Owner);
	return Runner;
}

void FScriptableAction::Run(UObject* InOwner, const FScriptableContext& InContext)
{
	if (!InOwner) return;

	SetContext(InContext);
	Run(InOwner);
}

UScriptableActionRunner* FScriptableAction::Run(FScriptableAction&& Action, UObject* Owner, const FScriptableContext& InContext)
{
	if (!Owner) return nullptr;

	Action.SetContext(InContext);
	return Run(MoveTemp(Action), Owner);
}

void FScriptableAction::Register(UObject* InOwner)
{
	Super::Register(InOwner);

	// Filter invalid tasks
	for (int32 i = Tasks.Num() - 1; i >= 0; --i)
	{
		if (!Tasks[i])
		{
			Tasks.RemoveAt(i);
		}
	}

	for (UScriptableTask* Task : Tasks)
	{
		if (Task)
		{
			// Add to local map and inject THIS Context into the task
			AddBindingSource(Task);

			if (Task->IsEnabled())
			{
				Task->Register(Owner);
			}
		}
	}
}

void FScriptableAction::Unregister()
{
	for (UScriptableTask* Task : Tasks)
	{
		if (Task && Task->IsEnabled())
		{
			Task->Unregister();
		}
	}

	Super::Unregister();
}

void FScriptableAction::Reset()
{
	// Reset logic state
	bIsRunning = false;
	CurrentTaskIndex = 0;

	// Propagate hard reset to all tasks
	for (UScriptableTask* Task : Tasks)
	{
		if (Task)
		{
			Task->Reset();
		}
	}

	Unregister();
}

void FScriptableAction::Begin()
{
	if (Tasks.IsEmpty())
	{
		Finish(true);
		return;
	}

	bIsRunning = true;
	CurrentTaskIndex = 0;

	if (Mode == EScriptableActionMode::Sequence)
	{
		BeginSubTask(Tasks[CurrentTaskIndex]);
	}
	else if (Mode == EScriptableActionMode::Parallel)
	{
		for (UScriptableTask* Task : Tasks)
		{
			BeginSubTask(Task);
		}
	}

	OnActionBegin.Broadcast();
}

void FScriptableAction::Finish(bool bForce)
{
	if (!bIsRunning && !bForce) return;

	// Clear running state up-front so any cascade (a Stop-triggered broadcast, a re-entrant Finish via
	// a sub-task's handler, etc.) sees us as already finishing and bails early.
	bIsRunning = false;
	CurrentTaskIndex = 0;

	// Snapshot the tasks: handlers fired during teardown could mutate the array under us.
	const TArray<TObjectPtr<UScriptableTask>> TasksSnapshot = Tasks;
	for (UScriptableTask* Task : TasksSnapshot)
	{
		// IsValid guards against a task whose outer was destroyed (e.g. PIE end while still running) and
		// whose delegates would be in an invalid state if touched.
		if (!IsValid(Task)) continue;

		Task->OnTaskFinishNative.RemoveAll(this);
		Task->OnTaskStoppedNative.RemoveAll(this);
		if (bForce)
		{
			Task->Stop();
		}
		else
		{
			Task->Finish();
		}
	}

	OnActionFinish.Broadcast();
}

void FScriptableAction::BeginSubTask(UScriptableTask* Task)
{
	if (!Task || !Task->IsEnabled())
	{
		OnSubTaskFinished(Task);
		return;
	}

	Task->OnTaskFinishNative.RemoveAll(this);
	Task->OnTaskStoppedNative.RemoveAll(this);
	Task->OnTaskFinishNative.AddRaw(this, &FScriptableAction::OnSubTaskFinished);
	Task->OnTaskStoppedNative.AddRaw(this, &FScriptableAction::OnSubTaskFinished);
	Task->Begin();
}

void FScriptableAction::OnSubTaskFinished(UScriptableTask* Task)
{
	if (Task)
	{
		Task->OnTaskFinishNative.RemoveAll(this);
		Task->OnTaskStoppedNative.RemoveAll(this);
	}

	// In Parallel mode CurrentTaskIndex acts as a counter
	if (++CurrentTaskIndex >= Tasks.Num())
	{
		Finish();
	}
	else if (Mode == EScriptableActionMode::Sequence)
	{
		BeginSubTask(Tasks[CurrentTaskIndex]);
	}
}
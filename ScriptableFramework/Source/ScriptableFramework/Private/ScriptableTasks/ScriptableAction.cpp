// Copyright 2026 kirzo

#include "ScriptableTasks/ScriptableAction.h"
#include "ScriptableTasks/ScriptableTask.h"
#include "ScriptableTasks/ScriptableActionRunner.h"
#include "ScriptableContext.h"
#include "UObject/UnrealType.h"

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

UScriptableActionRunner* FScriptableAction::RunCopy(UObject* Owner, const FScriptableAction& SourceAction)
{
	if (!Owner) return nullptr;

	// Redirect to the CDO copy when SourceAction is a member of Owner via an EditDefaultsOnly
	// property — otherwise we'd clone stale per-instance Instanced subobjects on placed-in-level
	// actors (the bug the async BP node already worked around).
	const FScriptableAction* Authoritative = ResolveAuthoritative(&SourceAction, Owner);
	if (!Authoritative) return nullptr;

	UScriptableActionRunner* Runner = NewObject<UScriptableActionRunner>(Owner);
	if (!Runner) return nullptr;

	FScriptableAction Cloned = Authoritative->Clone(Runner);
	Runner->Launch(MoveTemp(Cloned), Owner);
	return Runner;
}

UScriptableActionRunner* FScriptableAction::RunCopy(UObject* Owner, const FScriptableAction& SourceAction, const FScriptableContext& InContext)
{
	if (!Owner) return nullptr;

	const FScriptableAction* Authoritative = ResolveAuthoritative(&SourceAction, Owner);
	if (!Authoritative) return nullptr;

	UScriptableActionRunner* Runner = NewObject<UScriptableActionRunner>(Owner);
	if (!Runner) return nullptr;

	FScriptableAction Cloned = Authoritative->Clone(Runner);
	Cloned.SetContext(InContext);
	Runner->Launch(MoveTemp(Cloned), Owner);
	return Runner;
}

const FScriptableAction* FScriptableAction::ResolveAuthoritative(const FScriptableAction* InstanceAction, const UObject* Owner)
{
	if (!InstanceAction || !Owner) return InstanceAction;

	const uint8* OwnerBase = reinterpret_cast<const uint8*>(Owner);
	const uint8* ActionBase = reinterpret_cast<const uint8*>(InstanceAction);

	// Early out if the pointer is not physically inside the Owner's direct memory footprint.
	if (ActionBase < OwnerBase) return InstanceAction;

	const SIZE_T Offset = static_cast<SIZE_T>(ActionBase - OwnerBase);
	if (Offset >= static_cast<SIZE_T>(Owner->GetClass()->GetPropertiesSize()))
	{
		return InstanceAction;
	}

	// We only iterate properties that explicitly contain our memory offset
	bool bIsEditDefaultsOnly = false;
	const UStruct* CurrentStruct = Owner->GetClass();
	SIZE_T CurrentBaseOffset = 0;

	while (CurrentStruct)
	{
		// Reached the action itself — descending further would inspect FScriptableAction's own
		// internals, whose flags don't speak about the wrapper chain. Without this guard, an
		// EditDefaultsOnly UPROPERTY added inside FScriptableAction in the future would falsely
		// flip every RunCopy caller into the CDO path.
		if (CurrentStruct == FScriptableAction::StaticStruct()) break;

		bool bFoundInLevel = false;
		for (TFieldIterator<FProperty> It(CurrentStruct); It; ++It)
		{
			const FProperty* Prop = *It;
			SIZE_T PropStart = CurrentBaseOffset + static_cast<SIZE_T>(Prop->GetOffset_ForInternal());
			// GetSize() accounts for total memory (including static inline arrays)
			SIZE_T PropEnd = PropStart + static_cast<SIZE_T>(Prop->GetSize());

			if (Offset >= PropStart && Offset < PropEnd)
			{
				// If any property in the chain is blocked from instance editing, the action inherits it.
				if (Prop->HasAnyPropertyFlags(CPF_DisableEditOnInstance))
				{
					bIsEditDefaultsOnly = true;
				}

				if (const FStructProperty* StructProp = CastField<FStructProperty>(Prop))
				{
					// Adjust base offset for static inline arrays (e.g. FMyStruct Configs[4])
					SIZE_T ElementOffset = 0;
					if (Prop->ArrayDim > 1)
					{
						SIZE_T RelativeOffset = Offset - PropStart;
						SIZE_T ArrayIndex = RelativeOffset / static_cast<SIZE_T>(Prop->GetElementSize());
						ElementOffset = ArrayIndex * static_cast<SIZE_T>(Prop->GetElementSize());
					}

					CurrentStruct = StructProp->Struct;
					CurrentBaseOffset = PropStart + ElementOffset;
					bFoundInLevel = true;
				}
				else
				{
					CurrentStruct = nullptr; // Hit a non-struct leaf property (shouldn't happen for our target, but safe)
				}
				break;
			}
		}

		if (!bFoundInLevel) break;
	}

	// If the property chain is authoritative (EditDefaultsOnly), fetch it directly from the CDO.
	if (bIsEditDefaultsOnly)
	{
		if (const UObject* CDO = Owner->GetClass()->GetDefaultObject())
		{
			// We can bypass reflection entirely for the read and just cast the offset.
			return reinterpret_cast<const FScriptableAction*>(reinterpret_cast<const uint8*>(CDO) + Offset);
		}
	}

	return InstanceAction;
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
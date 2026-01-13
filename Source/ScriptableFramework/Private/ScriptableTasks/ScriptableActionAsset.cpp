// Copyright 2026 kirzo

#include "ScriptableTasks/ScriptableActionAsset.h"
#include "ScriptableTasks/ScriptableTask.h"

#include "Algo/AnyOf.h"

UE_DISABLE_OPTIMIZATION

#if WITH_EDITOR
void UScriptableActionAsset::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UScriptableActionAsset, Context))
	{
		Action.GetContext().MigrateToNewBagInstance(Context);
	}
}
#endif

void UScriptableTask_RunAsset::OnRegister()
{
	Super::OnRegister();

	if (!RuntimeAction.IsRunning())
	{
		InstantiateRuntimeAction();
	}
}

void UScriptableTask_RunAsset::OnUnregister()
{
	Super::OnUnregister();
	TeardownRuntimeAction();
}

void UScriptableTask_RunAsset::ResetTask()
{
	if (RuntimeAction.IsRunning())
	{
		RuntimeAction.Finish();
	}

	InstantiateRuntimeAction();
}

void UScriptableTask_RunAsset::BeginTask()
{
	if (Asset && !RuntimeAction.Tasks.IsEmpty())
	{
		RuntimeAction.Begin();
	}
	else
	{
		Finish();
	}
}

void UScriptableTask_RunAsset::FinishTask()
{
	// Ensure the inner action is stopped properly
	RuntimeAction.Finish();
	RuntimeAction.Unregister();
}

void UScriptableTask_RunAsset::InstantiateRuntimeAction()
{
	TeardownRuntimeAction();

	if (Asset)
	{
		// Copy the Struct
		RuntimeAction = Asset->Action;
		RuntimeAction.Context = *GetContext();

		// Deep Copy Tasks
		// The 'Tasks' array currently points to the Asset's archetype objects.
		for (int32 i = 0; i < RuntimeAction.Tasks.Num(); ++i)
		{
			UScriptableTask* TemplateTask = RuntimeAction.Tasks[i];
			if (TemplateTask)
			{
				UScriptableTask* NewTaskInstance = DuplicateObject<UScriptableTask>(TemplateTask, this);
				RuntimeAction.Tasks[i] = NewTaskInstance;
			}
		}

		RuntimeAction.Register(GetOwner());
	}
}

void UScriptableTask_RunAsset::TeardownRuntimeAction()
{
	if (RuntimeAction.IsRunning())
	{
		RuntimeAction.Finish();
	}

	RuntimeAction.Unregister();

	// Explicitly empty tasks to drop references to the instanced objects
	RuntimeAction.Tasks.Empty();
}

UE_ENABLE_OPTIMIZATION
// Copyright 2026 kirzo

#include "ScriptableTasks/ScriptableTask_NestedAction.h"

void UScriptableTask_NestedAction::OnRegister()
{
	Super::OnRegister();

	if (!RuntimeAction.IsRunning())
	{
		InstantiateRuntimeAction();
	}
}

void UScriptableTask_NestedAction::OnUnregister()
{
	Super::OnUnregister();

	TeardownRuntimeAction();
}

void UScriptableTask_NestedAction::ResetTask()
{
	if (RuntimeAction.IsRunning())
	{
		RuntimeAction.Finish();
	}

	InstantiateRuntimeAction();
}

void UScriptableTask_NestedAction::BeginTask()
{
	if (RuntimeAction.Tasks.IsEmpty())
	{
		Finish();
		return;
	}

	RuntimeAction.Begin();
}

void UScriptableTask_NestedAction::FinishTask()
{
	// Symmetric with RunAsset: ensure the inner action is fully torn down before we propagate.
	RuntimeAction.OnActionFinish.RemoveAll(this);

	if (RuntimeAction.IsRunning())
	{
		RuntimeAction.Finish();
	}
	RuntimeAction.Unregister();
}

void UScriptableTask_NestedAction::StopTask()
{
	// Force-finish propagates Stop down to every inner task. Unbind first so the cascade doesn't try
	// to re-enter Finish on this already-stopped outer task.
	RuntimeAction.OnActionFinish.RemoveAll(this);

	if (RuntimeAction.IsRunning())
	{
		RuntimeAction.Finish(/*bForce=*/true);
	}
}

void UScriptableTask_NestedAction::InstantiateRuntimeAction()
{
	TeardownRuntimeAction();

	// Shallow-copy the source struct, then deep-copy each Task under `this` so loop iterations get
	// isolated state and we don't mutate the editor template.
	RuntimeAction = Action;

	// Inherit the parent scope's context by copying its bag into our local one. This is what makes
	// the inner tasks resolve context bindings — RuntimeAction.Register passes the actor as Owner,
	// and AddBindingSource's UScriptableObject-cast fallback would fail on the actor, leaving inner
	// ContextRefs null. Mirrors UScriptableCondition_Asset.
	if (const FInstancedPropertyBag* ParentContext = GetContext())
	{
		RuntimeAction.Context = *ParentContext;
	}
	else
	{
		RuntimeAction.ResetContext();
	}

	// Inherit the parent scope's Locals when this nested unit doesn't declare its own. Cannot copy
	// because SetLocal writes have to propagate back to the parent's bag; instead point GetLocals at
	// the external bag. When the nested unit DOES declare its own LocalsDefinitions it stays isolated.
	if (RuntimeAction.LocalsDefinitions.Num() == 0)
	{
		RuntimeAction.SetInheritedLocals(GetMutableLocals());
	}

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

	RuntimeAction.OnActionFinish.AddUObject(this, &UScriptableTask_NestedAction::HandleActionFinished);
}

void UScriptableTask_NestedAction::TeardownRuntimeAction()
{
	RuntimeAction.OnActionFinish.RemoveAll(this);

	if (RuntimeAction.IsRunning())
	{
		RuntimeAction.Finish();
	}
	RuntimeAction.Unregister();

	// Drop references to the duplicated instances so the next Instantiate doesn't keep stale entries.
	RuntimeAction.Tasks.Empty();
}

void UScriptableTask_NestedAction::HandleActionFinished()
{
	// Defer to the standard task lifecycle. If we're in a loop, FinishWithOutput will call BeginTask
	// again which re-enters RuntimeAction.Begin on the cloned tasks (their Status is reset by Begin).
	Finish();
}

#if WITH_EDITOR
FText UScriptableTask_NestedAction::GetDisplayTitle() const
{
	if (!ActionName.IsEmpty())
	{
		return FText::FromString(ActionName);
	}
	return INVTEXT("Nested Action");
}
#endif

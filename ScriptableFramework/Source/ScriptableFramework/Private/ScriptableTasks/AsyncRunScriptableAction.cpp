// Copyright 2026 kirzo

#include "ScriptableTasks/AsyncRunScriptableAction.h"
#include "ScriptableTasks/ScriptableActionRunner.h"
#include "ScriptableTasks/ScriptableTask.h"
#include "UObject/UnrealType.h"

namespace
{
	/**
	 * Returns the authoritative action to clone. When the source action lives on a BP variable marked
	 * NOT InstanceEditable (CPF_DisableEditOnInstance), placed-in-level instances may keep stale
	 * per-instance state (Instanced subobjects that never re-propagated after a CDO edit). In that
	 * case we redirect to the same property on the class CDO, so the runner always sees defaults.
	 * Otherwise we honor whatever the caller passed.
	 */
	const FScriptableAction* ResolveAuthoritativeAction(const FScriptableAction* InstanceAction, const UObject* Owner)
	{
		if (!InstanceAction || !Owner) return InstanceAction;

		const uint8* OwnerBase = reinterpret_cast<const uint8*>(Owner);
		const uint8* ActionBase = reinterpret_cast<const uint8*>(InstanceAction);
		const SIZE_T Offset = static_cast<SIZE_T>(ActionBase - OwnerBase);

		// Bound the search to the owner's memory footprint so we don't iterate for foreign pointers.
		if (Offset >= static_cast<SIZE_T>(Owner->GetClass()->GetPropertiesSize())) return InstanceAction;

		for (TFieldIterator<FStructProperty> It(Owner->GetClass()); It; ++It)
		{
			const FStructProperty* StructProp = *It;
			if (!StructProp->Struct->IsChildOf(FScriptableAction::StaticStruct())) continue;
			if (static_cast<SIZE_T>(StructProp->GetOffset_ForInternal()) != Offset) continue;

			if (!StructProp->HasAllPropertyFlags(CPF_DisableEditOnInstance)) return InstanceAction;

			if (const UObject* CDO = Owner->GetClass()->GetDefaultObject())
			{
				return StructProp->ContainerPtrToValuePtr<FScriptableAction>(CDO);
			}
			break;
		}

		return InstanceAction;
	}
}

UAsyncRunScriptableAction* UAsyncRunScriptableAction::RunScriptableAction(UObject* Owner, FScriptableAction& Action, const FScriptableContext& Context)
{
	UAsyncRunScriptableAction* Node = NewObject<UAsyncRunScriptableAction>(Owner);

	Node->ActionOwner = Owner;
	Node->SourceAction = &Action;
	Node->LaunchContext = Context;

	if (Owner)
	{
		Node->RegisterWithGameInstance(Owner);
	}

	return Node;
}

void UAsyncRunScriptableAction::Activate()
{
	Super::Activate();

	if (!ActionOwner || !SourceAction)
	{
		SetReadyToDestroy();
		return;
	}

	// Spawn the runner and deep-copy the source action's tasks into it. Clone owns the new tasks via
	// the Runner so the source BP variable is never mutated and concurrent runs don't share state.
	Runner = NewObject<UScriptableActionRunner>(ActionOwner);
	if (!Runner)
	{
		SetReadyToDestroy();
		return;
	}

	// Redirect to the CDO when the property is EditDefaultsOnly so stale per-instance overrides
	// (e.g. Instanced tasks that didn't re-propagate to already-placed actors after a CDO edit)
	// don't sabotage the run.
	const FScriptableAction* EffectiveAction = ResolveAuthoritativeAction(SourceAction, ActionOwner);

	FScriptableAction ClonedAction = EffectiveAction->Clone(Runner);
	ClonedAction.SetContext(LaunchContext);
	Runner->Launch(MoveTemp(ClonedAction), ActionOwner);

	// Source pointer is consumed; the BP variable's lifetime is not ours.
	SourceAction = nullptr;

	// Hand the runner out immediately so callers can drive it (Cancel, IsRunning) while it runs.
	Started.Broadcast(Runner);

	// An empty-Tasks action finishes synchronously inside Launch, before we could subscribe.
	if (!Runner->IsRunning())
	{
		HandleActionFinished();
		return;
	}

	Runner->OnFinishedNative.AddUObject(this, &UAsyncRunScriptableAction::HandleActionFinished);
}

void UAsyncRunScriptableAction::HandleActionFinished()
{
	if (Runner)
	{
		Runner->OnFinishedNative.RemoveAll(this);
	}

	Finished.Broadcast(Runner);
	SetReadyToDestroy();
}

void UAsyncRunScriptableAction::SetReadyToDestroy()
{
	if (Runner)
	{
		Runner->OnFinishedNative.RemoveAll(this);
	}

	Runner = nullptr;
	ActionOwner = nullptr;
	SourceAction = nullptr;

	Super::SetReadyToDestroy();
}
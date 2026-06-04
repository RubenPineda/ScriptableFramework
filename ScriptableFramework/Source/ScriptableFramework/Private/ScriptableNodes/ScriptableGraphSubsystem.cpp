// Copyright 2026 kirzo

#include "ScriptableNodes/ScriptableGraphSubsystem.h"
#include "ScriptableNodes/ScriptableGraphInstance.h"
#include "ScriptableTasks/ScriptableActionRunner.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

UScriptableGraphSubsystem* UScriptableGraphSubsystem::Get(const UObject* WorldContext)
{
	if (!GEngine) return nullptr;

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull);
	return World ? World->GetSubsystem<UScriptableGraphSubsystem>() : nullptr;
}

UScriptableGraphInstance* UScriptableGraphSubsystem::RunGraph(const UObject* WorldContext, UScriptableGraph* Asset, UObject* Owner, const FScriptableContext& Context, FName Id)
{
	if (!Asset) return nullptr;

	UScriptableGraphSubsystem* Subsystem = Get(WorldContext);
	if (!Subsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("UScriptableGraphSubsystem::RunGraph: could not resolve a world/subsystem from '%s'; graph '%s' will not run."),
			*GetNameSafe(WorldContext), *GetNameSafe(Asset));
		return nullptr;
	}

	UScriptableGraphInstance* Runner = NewObject<UScriptableGraphInstance>(Owner ? Owner : const_cast<UObject*>(WorldContext));
	if (!Runner) return nullptr;

	Subsystem->RegisterRunner(Runner);
	Runner->Launch(Asset, Owner, Context, Id);
	return Runner;
}

void UScriptableGraphSubsystem::CancelAllRunners()
{
	// Iterate copies: each cancel routes through Finish, which unregisters from the live array.
	// Regular Cancel here (user-invoked path) — graph runners run their Exit cleanup if present,
	// action runners force-finish.
	{
		const TArray<TObjectPtr<UScriptableGraphInstance>> GraphsCopy = ActiveRunners;
		for (const TObjectPtr<UScriptableGraphInstance>& Runner : GraphsCopy)
		{
			if (Runner) Runner->Cancel();
		}
	}
	{
		const TArray<TObjectPtr<UScriptableActionRunner>> ActionsCopy = ActiveActionRunners;
		for (const TObjectPtr<UScriptableActionRunner>& Runner : ActionsCopy)
		{
			if (Runner) Runner->Cancel();
		}
	}
}

void UScriptableGraphSubsystem::CancelRunner(UScriptableGraphInstance* Runner)
{
	if (!Runner) return;
	if (!ActiveRunners.Contains(Runner)) return;
	Runner->Cancel();
}

void UScriptableGraphSubsystem::CancelRunnersById(FName Id)
{
	if (Id.IsNone()) return;

	/** Snapshot copy: Cancel routes through Finish → Unregister, which mutates ActiveRunners. */
	const TArray<TObjectPtr<UScriptableGraphInstance>> Copy = ActiveRunners;
	for (const TObjectPtr<UScriptableGraphInstance>& Runner : Copy)
	{
		if (Runner && Runner->GetId() == Id) Runner->Cancel();
	}
}

void UScriptableGraphSubsystem::CancelRunnersForOwner(UObject* Owner)
{
	if (!Owner) return;

	// Snapshot first because Cancel → Finish → Unregister removes entries mid-iteration. Both runner
	// types are filtered by their stored Launch owner (a private field; subsystem is friend of both).
	{
		const TArray<TObjectPtr<UScriptableGraphInstance>> GraphsCopy = ActiveRunners;
		for (const TObjectPtr<UScriptableGraphInstance>& Runner : GraphsCopy)
		{
			if (Runner && Runner->Owner == Owner) Runner->Cancel();
		}
	}
	{
		const TArray<TObjectPtr<UScriptableActionRunner>> ActionsCopy = ActiveActionRunners;
		for (const TObjectPtr<UScriptableActionRunner>& Runner : ActionsCopy)
		{
			if (Runner && Runner->Owner == Owner) Runner->Cancel();
		}
	}
}

TArray<UScriptableGraphInstance*> UScriptableGraphSubsystem::GetActiveRunners() const
{
	TArray<UScriptableGraphInstance*> Result;
	Result.Reserve(ActiveRunners.Num());
	for (const TObjectPtr<UScriptableGraphInstance>& Runner : ActiveRunners)
	{
		if (Runner) Result.Add(Runner);
	}
	return Result;
}

void UScriptableGraphSubsystem::RegisterRunner(UScriptableGraphInstance* Runner)
{
	if (Runner)
	{
		ActiveRunners.AddUnique(Runner);
	}
}

void UScriptableGraphSubsystem::UnregisterRunner(UScriptableGraphInstance* Runner)
{
	if (Runner)
	{
		ActiveRunners.RemoveSingleSwap(Runner);
	}
}

void UScriptableGraphSubsystem::RegisterActionRunner(UScriptableActionRunner* Runner)
{
	if (Runner)
	{
		ActiveActionRunners.AddUnique(Runner);
	}
}

void UScriptableGraphSubsystem::UnregisterActionRunner(UScriptableActionRunner* Runner)
{
	if (Runner)
	{
		ActiveActionRunners.RemoveSingleSwap(Runner);
	}
}

void UScriptableGraphSubsystem::CancelAllForTeardown()
{
	// World teardown: graphs use CancelImmediate (skip Exit cleanup, the world and any actors it
	// would touch are being destroyed); actions still use Cancel since they have no equivalent
	// world-touching cleanup hook.
	{
		const TArray<TObjectPtr<UScriptableGraphInstance>> GraphsCopy = ActiveRunners;
		for (const TObjectPtr<UScriptableGraphInstance>& Runner : GraphsCopy)
		{
			if (Runner) Runner->CancelImmediate();
		}
		ActiveRunners.Reset();
	}
	{
		const TArray<TObjectPtr<UScriptableActionRunner>> ActionsCopy = ActiveActionRunners;
		for (const TObjectPtr<UScriptableActionRunner>& Runner : ActionsCopy)
		{
			if (Runner) Runner->Cancel();
		}
		ActiveActionRunners.Reset();
	}
}

void UScriptableGraphSubsystem::Deinitialize()
{
	CancelAllForTeardown();
	Super::Deinitialize();
}

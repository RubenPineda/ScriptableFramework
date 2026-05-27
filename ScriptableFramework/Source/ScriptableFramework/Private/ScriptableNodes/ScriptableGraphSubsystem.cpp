// Copyright 2026 kirzo

#include "ScriptableNodes/ScriptableGraphSubsystem.h"
#include "ScriptableNodes/ScriptableGraphInstance.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

UScriptableGraphSubsystem* UScriptableGraphSubsystem::Get(const UObject* WorldContext)
{
	if (!GEngine) return nullptr;

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull);
	return World ? World->GetSubsystem<UScriptableGraphSubsystem>() : nullptr;
}

UScriptableGraphInstance* UScriptableGraphSubsystem::RunGraph(const UObject* WorldContext, UScriptableGraph* Asset, UObject* Owner, const FScriptableContext& Context)
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
	Runner->Launch(Asset, Owner, Context);
	return Runner;
}

void UScriptableGraphSubsystem::CancelAllRunners()
{
	// Iterate a copy: each Cancel() routes through Finish(), which unregisters from ActiveRunners.
	const TArray<TObjectPtr<UScriptableGraphInstance>> RunnersCopy = ActiveRunners;
	for (const TObjectPtr<UScriptableGraphInstance>& Runner : RunnersCopy)
	{
		if (Runner)
		{
			Runner->Cancel();
		}
	}

	ActiveRunners.Reset();
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

void UScriptableGraphSubsystem::Deinitialize()
{
	CancelAllRunners();
	Super::Deinitialize();
}

// Copyright 2026 kirzo

#include "ScriptableTasks/ScriptableTask_RunGraph.h"
#include "ScriptableNodes/ScriptableGraph.h"
#include "ScriptableNodes/ScriptableGraphInstance.h"
#include "ScriptableContext.h"

void UScriptableTask_RunGraph::BeginTask()
{
	if (!GraphAsset)
	{
		Finish();
		return;
	}

	// Defensive: tearing down an old runner if for some reason we still have one.
	if (SubRunner)
	{
		SubRunner->OnGraphFinishedNative.RemoveAll(this);
		SubRunner = nullptr;
	}

	SubRunner = NewObject<UScriptableGraphInstance>(this);
	SubRunner->OnGraphFinishedNative.AddUObject(this, &UScriptableTask_RunGraph::OnSubGraphFinished);

	SubRunner->Launch(GraphAsset, GetOwner(), FScriptableContext(GetContext()));
}

void UScriptableTask_RunGraph::FinishTask()
{
	CancelSubRunnerIfAny();
}

void UScriptableTask_RunGraph::StopTask()
{
	CancelSubRunnerIfAny();
}

void UScriptableTask_RunGraph::ResetTask()
{
	CancelSubRunnerIfAny();
}

void UScriptableTask_RunGraph::CancelSubRunnerIfAny()
{
	if (!SubRunner)
	{
		return;
	}

	SubRunner->OnGraphFinishedNative.RemoveAll(this);

	if (!SubRunner->IsFinished())
	{
		SubRunner->Cancel();
	}

	SubRunner = nullptr;
}

void UScriptableTask_RunGraph::OnSubGraphFinished()
{
	SubRunner = nullptr;
	Finish();
}

#if WITH_EDITOR
FText UScriptableTask_RunGraph::GetDisplayTitle() const
{
	if (GraphAsset)
	{
		return FText::FromString(FString::Printf(TEXT("Run Graph: %s"), *GraphAsset->GetName()));
	}
	return INVTEXT("Run Graph");
}
#endif
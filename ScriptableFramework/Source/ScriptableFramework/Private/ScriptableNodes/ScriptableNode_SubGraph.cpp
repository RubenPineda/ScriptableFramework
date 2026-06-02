// Copyright 2026 kirzo

#include "ScriptableNodes/ScriptableNode_SubGraph.h"
#include "ScriptableNodes/ScriptableGraph.h"
#include "ScriptableNodes/ScriptableGraphInstance.h"
#include "ScriptableNodes/ScriptableGraphSubsystem.h"
#include "ScriptableNodes/ScriptableNode_Exit.h"
#include "ScriptableContext.h"

const FName UScriptableNode_SubGraph::InInputName = TEXT("In");
const FName UScriptableNode_SubGraph::PendingOutputName = TEXT("__SubGraphPending__");

TArray<FName> UScriptableNode_SubGraph::GetInputPins() const
{
	return { InInputName };
}

TArray<FName> UScriptableNode_SubGraph::GetDeclaredOutputPins() const
{
	TArray<FName> Names = { UScriptableNode_Exit::FinishedOutputName, UScriptableNode_Exit::CancelledOutputName };

	if (SubGraphAsset)
	{
		for (const FName& UserOutput : SubGraphAsset->Outputs)
		{
			if (!UserOutput.IsNone()) Names.AddUnique(UserOutput);
		}
	}

	return Names;
}

void UScriptableNode_SubGraph::ProcessInput(FName InputName)
{
	if (InputName != InInputName) return;
	MarkInputInactive(InputName);

	if (!SubGraphAsset)
	{
		FinishImmediately(UScriptableNode_Exit::FinishedOutputName);
		return;
	}

	UObject* RunnerOwner = GetOwner();
	if (!RunnerOwner)
	{
		FinishImmediately(UScriptableNode_Exit::FinishedOutputName);
		return;
	}

	// Hidden "pending" output keeps the node in ActiveNodes until the sub-runner finishes.
	MarkOutputActive(PendingOutputName);

	// Passthrough context: parent values reach the sub-graph by name.
	FScriptableContext PassthroughContext;
	if (const FInstancedPropertyBag* ParentContext = GetContext())
	{
		PassthroughContext.GetBag() = *ParentContext;
	}

	RuntimeSubInstance = UScriptableGraphSubsystem::RunGraph(RunnerOwner, SubGraphAsset, RunnerOwner, PassthroughContext);
	if (!RuntimeSubInstance)
	{
		MarkOutputInactive(PendingOutputName);
		FinishImmediately(UScriptableNode_Exit::FinishedOutputName);
		return;
	}

	RuntimeSubInstance->OnGraphFinishedNative.AddUObject(this, &UScriptableNode_SubGraph::HandleSubGraphFinished);
}

void UScriptableNode_SubGraph::HandleSubGraphFinished()
{
	MarkOutputInactive(PendingOutputName);

	FName OutputToFire = UScriptableNode_Exit::FinishedOutputName;
	if (IsValid(RuntimeSubInstance))
	{
		const FName SubOutput = RuntimeSubInstance->GetCompletionOutput();
		if (!SubOutput.IsNone())
		{
			OutputToFire = SubOutput;
		}
		else if (RuntimeSubInstance->IsCancelled())
		{
			OutputToFire = UScriptableNode_Exit::CancelledOutputName;
		}

		RuntimeSubInstance->OnGraphFinishedNative.RemoveAll(this);
		RuntimeSubInstance = nullptr;
	}

	// Guard against stale outputs (asset changed after spawn): unknown name → "Finished".
	const TArray<FName> Declared = GetDeclaredOutputPins();
	if (!Declared.Contains(OutputToFire))
	{
		OutputToFire = UScriptableNode_Exit::FinishedOutputName;
	}

	MarkOutputActive(OutputToFire);
	FireOutput(OutputToFire);
}

void UScriptableNode_SubGraph::Teardown()
{
	if (IsValid(RuntimeSubInstance))
	{
		RuntimeSubInstance->OnGraphFinishedNative.RemoveAll(this);
		if (!RuntimeSubInstance->IsFinished())
		{
			RuntimeSubInstance->Cancel();
		}
		RuntimeSubInstance = nullptr;
	}

	DeactivateAllOutputs();
}

void UScriptableNode_SubGraph::FinishImmediately(FName OutputName)
{
	MarkOutputActive(OutputName);
	FireOutput(OutputName);
}

#if WITH_EDITOR
FText UScriptableNode_SubGraph::GetDisplayTitle() const
{
	if (SubGraphAsset)
	{
		return FText::Format(INVTEXT("Sub-Graph: {0}"), FText::FromString(SubGraphAsset->GetName()));
	}
	return INVTEXT("Sub-Graph");
}
#endif

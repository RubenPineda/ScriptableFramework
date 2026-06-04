// Copyright 2026 kirzo

#include "ScriptableNodes/ScriptableNode_SubGraph.h"
#include "ScriptableNodes/ScriptableGraph.h"
#include "ScriptableNodes/ScriptableGraphInstance.h"
#include "ScriptableNodes/ScriptableGraphSubsystem.h"
#include "ScriptableNodes/ScriptableNode_Exit.h"
#include "ScriptableNodes/ScriptableNode_ReceiveEvent.h"
#include "ScriptableObject.h"
#include "ScriptableContext.h"

const FName UScriptableNode_SubGraph::InInputName = TEXT("In");
const FName UScriptableNode_SubGraph::CancelInputName = TEXT("Cancel");
const FName UScriptableNode_SubGraph::PendingOutputName = TEXT("__SubGraphPending__");

TArray<FName> UScriptableNode_SubGraph::GetInputPins() const
{
	TArray<FName> Inputs = { InInputName };

	if (SubGraphAsset)
	{
		/** Every named ReceiveEvent the sub-asset declares becomes an input pin. Duplicates collapsed because
		 * FireEvent already broadcasts to every matching node — one pin suffices to trigger them all.
		 * Inputs named "In"/"Cancel" are skipped to avoid clashing with the built-in pins. */
		TSet<FName> Seen;
		for (const TObjectPtr<UScriptableNode>& Node : SubGraphAsset->Nodes)
		{
			const UScriptableNode_ReceiveEvent* Event = Cast<UScriptableNode_ReceiveEvent>(Node);
			if (!Event || Event->EventName.IsNone()) continue;
			if (Event->EventName == InInputName || Event->EventName == CancelInputName) continue;
			if (Seen.Contains(Event->EventName)) continue;
			Seen.Add(Event->EventName);
			Inputs.Add(Event->EventName);
		}
	}

	Inputs.Add(CancelInputName);

	return Inputs;
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
	MarkInputInactive(InputName);

	/** Cancel input: tear down the live sub-runner. HandleSubGraphFinished routes to "Cancelled" via the runner's IsCancelled() state. */
	if (InputName == CancelInputName)
	{
		if (IsValid(RuntimeSubInstance) && !RuntimeSubInstance->IsFinished())
		{
			RuntimeSubInstance->Cancel();
		}
		return;
	}

	const bool bIsEventInput = (InputName != InInputName);
	const bool bSubRunning = IsValid(RuntimeSubInstance) && RuntimeSubInstance->IsRunning();

	/** Event inputs auto-start the sub-graph if it isn't running yet. Any input acts as a start trigger. */
	if (!bSubRunning)
	{
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

	/** If the trigger was an event pin, fan it out now that the sub-runner is alive. */
	if (bIsEventInput && IsValid(RuntimeSubInstance) && RuntimeSubInstance->IsRunning())
	{
		RuntimeSubInstance->FireEvent(InputName);
	}
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

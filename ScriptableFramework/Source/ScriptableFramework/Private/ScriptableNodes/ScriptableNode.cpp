// Copyright 2026 kirzo

#include "ScriptableNodes/ScriptableNode.h"

const FName UScriptableNode::StopInputName = TEXT("Stop");
const FName UScriptableNode::StoppedOutputName = TEXT("Stopped");

TArray<FName> UScriptableNode::GetOutputPins() const
{
	TArray<FName> Outputs = GetDeclaredOutputPins();

	// Auto-append the Stopped output when the node accepts Stop.
	const TArray<FName> Inputs = GetInputPins();
	if (Inputs.Contains(StopInputName) && !Outputs.Contains(StoppedOutputName))
	{
		Outputs.Add(StoppedOutputName);
	}

	return Outputs;
}

void UScriptableNode::ActivateInput(FName InputName)
{
	ActiveInputPins.Add(InputName);

	if (CanProcessInput(InputName))
	{
		ProcessInput(InputName);
	}
}

void UScriptableNode::MarkInputInactive(FName InputName)
{
	ActiveInputPins.Remove(InputName);
	NotifyIfInactive();
}

void UScriptableNode::MarkOutputActive(FName OutputName)
{
	ActiveOutputPins.Add(OutputName);
}

void UScriptableNode::MarkOutputInactive(FName OutputName)
{
	ActiveOutputPins.Remove(OutputName);
	NotifyIfInactive();
}

void UScriptableNode::FireOutput(FName OutputName)
{
	// Broadcast first so the runner can enqueue downstream activations while this output is still "active".
	OnPinFiredNative.Broadcast(this, OutputName);

	// Then mark inactive and check for completion.
	ActiveOutputPins.Remove(OutputName);
	NotifyIfInactive();
}

void UScriptableNode::DeactivateAllOutputs()
{
	if (ActiveOutputPins.IsEmpty()) return;

	ActiveOutputPins.Reset();
	NotifyIfInactive();
}

void UScriptableNode::NotifyIfInactive()
{
	if (!IsNodeActive())
	{
		OnNodeInactiveNative.Broadcast(this);
	}
}
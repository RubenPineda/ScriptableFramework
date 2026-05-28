// Copyright 2026 kirzo

#include "ScriptableNodes/ScriptableNode_GoTo.h"

const FName UScriptableNode_GoTo::InInputName = TEXT("In");

TArray<FName> UScriptableNode_GoTo::GetInputPins() const
{
	return { InInputName };
}

TArray<FName> UScriptableNode_GoTo::GetDeclaredOutputPins() const
{
	// Terminal jump: the target ReceiveEvent continues the flow, so this node fires no output.
	return {};
}

void UScriptableNode_GoTo::ProcessInput(FName InputName)
{
	if (InputName != InInputName) return;

	MarkInputInactive(InInputName);

	// Hand the event to the runner; it wakes every matching ReceiveEvent node.
	if (!TargetEvent.IsNone())
	{
		OnRequestEventNative.Broadcast(TargetEvent);
	}
}

#if WITH_EDITOR
FText UScriptableNode_GoTo::GetDisplayTitle() const
{
	if (TargetEvent.IsNone() || TargetEvent.ToString().IsEmpty())
	{
		return INVTEXT("Go To (unset)");
	}
	return FText::Format(INVTEXT("Go to {0}"), FText::FromName(TargetEvent));
}
#endif

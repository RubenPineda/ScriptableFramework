// Copyright 2026 kirzo

#include "ScriptableNodes/ScriptableNode_ReceiveEvent.h"

const FName UScriptableNode_ReceiveEvent::OutOutputName = TEXT("Out");

TArray<FName> UScriptableNode_ReceiveEvent::GetInputPins() const
{
	return {};
}

TArray<FName> UScriptableNode_ReceiveEvent::GetDeclaredOutputPins() const
{
	return { OutOutputName };
}

void UScriptableNode_ReceiveEvent::Trigger()
{
	MarkOutputActive(OutOutputName);
	FireOutput(OutOutputName);
}

#if WITH_EDITOR
FText UScriptableNode_ReceiveEvent::GetDisplayTitle() const
{
	if (EventName.IsNone() || EventName.ToString().IsEmpty())
	{
		return INVTEXT("ReceiveEvent (unnamed)");
	}
	return FText::FromName(EventName);
}
#endif
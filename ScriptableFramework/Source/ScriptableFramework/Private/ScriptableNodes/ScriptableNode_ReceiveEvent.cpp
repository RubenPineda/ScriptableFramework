// Copyright 2026 kirzo

#include "ScriptableNodes/ScriptableNode_ReceiveEvent.h"
#include "ScriptableNodes/ScriptableNode_GoTo.h"
#include "ScriptableNodes/ScriptableGraph.h"

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

void UScriptableNode_ReceiveEvent::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);
	if (PropertyAboutToChange && PropertyAboutToChange->GetFName() == GET_MEMBER_NAME_CHECKED(UScriptableNode_ReceiveEvent, EventName))
	{
		PreviousEventName_ForRename = EventName;
	}
}

void UScriptableNode_ReceiveEvent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UScriptableNode_ReceiveEvent, EventName))
	{
		if (UScriptableGraph* Graph = GetTypedOuter<UScriptableGraph>())
		{
			ApplyTargetReferenceRename(Graph, PreviousEventName_ForRename, EventName);
		}
		PreviousEventName_ForRename = NAME_None;
	}
}

void UScriptableNode_ReceiveEvent::ApplyTargetReferenceRename(UScriptableGraph* Graph, FName OldName, FName NewName)
{
	if (!Graph || OldName.IsNone() || OldName == NewName) return;

	for (const TObjectPtr<UScriptableNode>& Node : Graph->Nodes)
	{
		UScriptableNode_GoTo* GoTo = Cast<UScriptableNode_GoTo>(Node);
		if (!GoTo) continue;
		if (GoTo->TargetEvent != OldName) continue;

		GoTo->Modify();
		GoTo->TargetEvent = NewName;
	}
}
#endif
// Copyright 2026 kirzo

#include "ScriptableNodes/ScriptableNode_Finish.h"
#include "ScriptableNodes/ScriptableNode_Exit.h"

const FName UScriptableNode_Finish::InInputName = TEXT("In");

TArray<FName> UScriptableNode_Finish::GetInputPins() const
{
	return { InInputName };
}

void UScriptableNode_Finish::ProcessInput(FName InputName)
{
	if (InputName != InInputName) return;
	MarkInputInactive(InputName);

	// Default to "Finished" so an unset Finish doesn't no-op into a stuck graph.
	const FName Effective = OutputName.IsNone() ? UScriptableNode_Exit::FinishedOutputName : OutputName;
	OnRequestFinishGraphNative.Broadcast(Effective);
}

#if WITH_EDITOR
FText UScriptableNode_Finish::GetDisplayTitle() const
{
	if (OutputName.IsNone()) return INVTEXT("Finish");
	return FText::Format(INVTEXT("Finish ({0})"), FText::FromName(OutputName));
}
#endif

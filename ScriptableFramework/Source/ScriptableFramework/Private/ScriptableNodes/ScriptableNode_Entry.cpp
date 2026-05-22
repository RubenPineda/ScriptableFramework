// Copyright 2026 kirzo

#include "ScriptableNodes/ScriptableNode_Entry.h"

const FName UScriptableNode_Entry::OutOutputName = TEXT("Out");

void UScriptableNode_Entry::Activate()
{
	MarkOutputActive(OutOutputName);
	FireOutput(OutOutputName);
}
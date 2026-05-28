// Copyright 2026 kirzo

#include "ScriptableNodes/ScriptableNode_Exit.h"

const FName UScriptableNode_Exit::FinishedOutputName = TEXT("Finished");
const FName UScriptableNode_Exit::CancelledOutputName = TEXT("Cancelled");

TArray<FName> UScriptableNode_Exit::GetDeclaredOutputPins() const
{
	return { FinishedOutputName, CancelledOutputName };
}

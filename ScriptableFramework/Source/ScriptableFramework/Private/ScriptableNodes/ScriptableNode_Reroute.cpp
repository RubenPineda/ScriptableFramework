// Copyright 2026 kirzo

#include "ScriptableNodes/ScriptableNode_Reroute.h"

namespace
{
	const FName InPinName(TEXT("In"));
	const FName OutPinName(TEXT("Out"));
}

TArray<FName> UScriptableNode_Reroute::GetInputPins() const
{
	return { InPinName };
}

TArray<FName> UScriptableNode_Reroute::GetDeclaredOutputPins() const
{
	return { OutPinName };
}

void UScriptableNode_Reroute::ProcessInput(FName InputName)
{
	FireOutput(OutPinName);
}
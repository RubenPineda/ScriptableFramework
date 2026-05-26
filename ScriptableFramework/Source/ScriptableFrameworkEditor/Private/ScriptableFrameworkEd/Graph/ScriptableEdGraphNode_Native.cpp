// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode_Native.h"
#include "ScriptableFrameworkEditorStyle.h"
#include "ScriptableNodes/ScriptableNode.h"

FLinearColor UScriptableEdGraphNode_Native::GetNodeTitleColor() const
{
	return FScriptableFrameworkEditorStyle::ScriptableSystemNodeColor;
}

bool UScriptableEdGraphNode_Native::ShouldShowPinLabel(FName PinName) const
{
	const UScriptableNode* Runtime = GetRuntimeNode();
	if (!Runtime) return Super::ShouldShowPinLabel(PinName);

	const TArray<FName> Inputs = Runtime->GetInputPins();
	if (Inputs.Num() == 1 && Inputs[0] == PinName) return false;

	const TArray<FName> Outputs = Runtime->GetOutputPins();
	if (Outputs.Num() == 1 && Outputs[0] == PinName) return false;

	return Super::ShouldShowPinLabel(PinName);
}
// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode.h"
#include "ScriptableNodes/ScriptableNode.h"

const FName UScriptableEdGraphNode::ScriptableExecPinCategory = TEXT("ScriptableExec");

void UScriptableEdGraphNode::AllocateDefaultPins()
{
	if (!RuntimeNode) return;

	for (const FName& InputName : RuntimeNode->GetInputPins())
	{
		CreatePin(EGPD_Input, ScriptableExecPinCategory, InputName);
	}

	for (const FName& OutputName : RuntimeNode->GetOutputPins())
	{
		CreatePin(EGPD_Output, ScriptableExecPinCategory, OutputName);
	}
}

FText UScriptableEdGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (RuntimeNode)
	{
		return RuntimeNode->GetClass()->GetDisplayNameText();
	}
	return NSLOCTEXT("ScriptableEdGraphNode", "DefaultTitle", "Scriptable Node");
}

FLinearColor UScriptableEdGraphNode::GetNodeTitleColor() const
{
	return FLinearColor(0.4f, 0.4f, 0.4f);
}
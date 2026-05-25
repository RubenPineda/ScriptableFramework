// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/ScriptableGraphNodeFactory.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode_Task.h"
#include "ScriptableFrameworkEd/Graph/SScriptableGraphNode_Task.h"

TSharedPtr<SGraphNode> FScriptableGraphNodeFactory::CreateNode(UEdGraphNode* Node) const
{
	if (UScriptableEdGraphNode_Task* TaskNode = Cast<UScriptableEdGraphNode_Task>(Node))
	{
		return SNew(SScriptableGraphNode_Task, TaskNode);
	}

	return nullptr;
}
// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/ScriptableGraphNodeFactory.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode_Task.h"
#include "ScriptableFrameworkEd/Graph/Nodes/ScriptableEdGraphNode_Sequence.h"
#include "ScriptableFrameworkEd/Graph/Nodes/ScriptableEdGraphNode_AND.h"
#include "ScriptableFrameworkEd/Graph/Nodes/ScriptableEdGraphNode_Reroute.h"
#include "ScriptableFrameworkEd/Graph/Nodes/ScriptableEdGraphNode_GoTo.h"
#include "ScriptableFrameworkEd/Graph/Widgets/SScriptableGraphNode_Task.h"
#include "ScriptableFrameworkEd/Graph/Widgets/SScriptableGraphNode_Sequence.h"
#include "ScriptableFrameworkEd/Graph/Widgets/SScriptableGraphNode_AND.h"
#include "ScriptableFrameworkEd/Graph/Widgets/SScriptableGraphNode_GoTo.h"
#include "SGraphNodeKnot.h"

TSharedPtr<SGraphNode> FScriptableGraphNodeFactory::CreateNode(UEdGraphNode* Node) const
{
	if (UScriptableEdGraphNode_Task* TaskNode = Cast<UScriptableEdGraphNode_Task>(Node))
	{
		return SNew(SScriptableGraphNode_Task, TaskNode);
	}

	if (UScriptableEdGraphNode_Sequence* SequenceNode = Cast<UScriptableEdGraphNode_Sequence>(Node))
	{
		return SNew(SScriptableGraphNode_Sequence, SequenceNode);
	}

	if (UScriptableEdGraphNode_AND* ANDNode = Cast<UScriptableEdGraphNode_AND>(Node))
	{
		return SNew(SScriptableGraphNode_AND, ANDNode);
	}

	if (UScriptableEdGraphNode_Reroute* RerouteNode = Cast<UScriptableEdGraphNode_Reroute>(Node))
	{
		return SNew(SGraphNodeKnot, RerouteNode);
	}

	if (UScriptableEdGraphNode_GoTo* GoToNode = Cast<UScriptableEdGraphNode_GoTo>(Node))
	{
		return SNew(SScriptableGraphNode_GoTo, GoToNode);
	}

	return nullptr;
}
// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/Nodes/ScriptableEdGraphNode_SubGraph.h"
#include "ScriptableNodes/ScriptableNode_SubGraph.h"
#include "ScriptableFrameworkEditorStyle.h"

UScriptableEdGraphNode_SubGraph::UScriptableEdGraphNode_SubGraph()
{
	RuntimeNodeClass = UScriptableNode_SubGraph::StaticClass();
}

FLinearColor UScriptableEdGraphNode_SubGraph::GetNodeTitleColor() const
{
	// Same teal as the UScriptableGraph asset entry — signals the node embeds a graph.
	return FScriptableFrameworkEditorStyle::ScriptableGraphColor;
}

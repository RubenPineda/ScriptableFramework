// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/Nodes/ScriptableEdGraphNode_Reroute.h"
#include "ScriptableNodes/ScriptableNode_Reroute.h"

UScriptableEdGraphNode_Reroute::UScriptableEdGraphNode_Reroute()
{
	RuntimeNodeClass = UScriptableNode_Reroute::StaticClass();
}

FText UScriptableEdGraphNode_Reroute::GetTooltipText() const
{
	return INVTEXT("Reroute — passthrough used to organize wires.");
}
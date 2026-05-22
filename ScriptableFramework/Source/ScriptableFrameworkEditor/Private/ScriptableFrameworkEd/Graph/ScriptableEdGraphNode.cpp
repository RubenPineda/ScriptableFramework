// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableNodes/ScriptableGraph.h"
#include "EdGraph/EdGraph.h"

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

void UScriptableEdGraphNode::DestroyNode()
{
	// Capture the asset reference before Super::DestroyNode unhooks us from the graph.
	UEdGraph* OwningGraph = GetGraph();
	UScriptableGraph* GraphAsset = OwningGraph ? Cast<UScriptableGraph>(OwningGraph->GetOuter()) : nullptr;
	UScriptableNode* RuntimeNodeToRemove = RuntimeNode;

	Super::DestroyNode();

	// Entry node is auto-repaired by UScriptableGraph::EnsureEntryNode; never strip it from the asset.
	if (!GraphAsset || !RuntimeNodeToRemove) return;
	if (RuntimeNodeToRemove->GetBindingID() == GraphAsset->EntryNodeID) return;

	GraphAsset->Modify();
	GraphAsset->Nodes.Remove(RuntimeNodeToRemove);
}
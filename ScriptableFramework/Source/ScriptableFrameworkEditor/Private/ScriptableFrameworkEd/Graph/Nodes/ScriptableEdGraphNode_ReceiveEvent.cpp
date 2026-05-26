// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/Nodes/ScriptableEdGraphNode_ReceiveEvent.h"
#include "ScriptableFrameworkEditorStyle.h"
#include "ScriptableNodes/ScriptableNode_ReceiveEvent.h"

#include "Styling/AppStyle.h"

UScriptableEdGraphNode_ReceiveEvent::UScriptableEdGraphNode_ReceiveEvent()
{
	RuntimeNodeClass = UScriptableNode_ReceiveEvent::StaticClass();
}

FLinearColor UScriptableEdGraphNode_ReceiveEvent::GetNodeTitleColor() const
{
	return FScriptableFrameworkEditorStyle::ScriptableEntryNodeColor;
}

FSlateIcon UScriptableEdGraphNode_ReceiveEvent::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = FLinearColor::White;
	return FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.Event_16x");
}
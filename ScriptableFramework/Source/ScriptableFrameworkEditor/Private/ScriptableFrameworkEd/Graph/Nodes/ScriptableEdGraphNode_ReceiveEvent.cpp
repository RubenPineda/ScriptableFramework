// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/Nodes/ScriptableEdGraphNode_ReceiveEvent.h"
#include "ScriptableFrameworkEditorStyle.h"
#include "ScriptableNodes/ScriptableNode_ReceiveEvent.h"

#include "Styling/AppStyle.h"

UScriptableEdGraphNode_ReceiveEvent::UScriptableEdGraphNode_ReceiveEvent()
{
	RuntimeNodeClass = UScriptableNode_ReceiveEvent::StaticClass();
}

FText UScriptableEdGraphNode_ReceiveEvent::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	const UScriptableNode_ReceiveEvent* Event = Cast<UScriptableNode_ReceiveEvent>(RuntimeNode);
	if (!Event)
	{
		return Super::GetNodeTitle(TitleType);
	}

	// Editing (F2) starts from the raw event name (blank when unset); otherwise show the display title.
	if (TitleType == ENodeTitleType::EditableTitle)
	{
		return Event->EventName.IsNone() ? FText::GetEmpty() : FText::FromName(Event->EventName);
	}
	return Event->GetDisplayTitle();
}

void UScriptableEdGraphNode_ReceiveEvent::OnRenameNode(const FString& NewName)
{
	if (UScriptableNode_ReceiveEvent* Event = Cast<UScriptableNode_ReceiveEvent>(RuntimeNode))
	{
		Event->Modify();
		Event->EventName = FName(*NewName);
	}
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
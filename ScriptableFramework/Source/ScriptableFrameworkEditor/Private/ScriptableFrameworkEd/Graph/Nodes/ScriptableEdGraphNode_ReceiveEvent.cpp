// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/Nodes/ScriptableEdGraphNode_ReceiveEvent.h"
#include "ScriptableFrameworkEditorStyle.h"
#include "ScriptableNodes/ScriptableNode_ReceiveEvent.h"
#include "ScriptableNodes/ScriptableGraph.h"

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
	UScriptableNode_ReceiveEvent* Event = Cast<UScriptableNode_ReceiveEvent>(RuntimeNode);
	if (!Event) return;

	const FName OldName = Event->EventName;
	const FName NewNameAsName(*NewName);
	if (OldName == NewNameAsName) return;

	Event->Modify();
	Event->EventName = NewNameAsName;

	/** Same refactor path the details panel uses (via PreEdit/PostEditChange) — keeps GoTos pointing at the new name. */
	if (UScriptableGraph* Graph = Event->GetTypedOuter<UScriptableGraph>())
	{
		UScriptableNode_ReceiveEvent::ApplyTargetReferenceRename(Graph, OldName, NewNameAsName);
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
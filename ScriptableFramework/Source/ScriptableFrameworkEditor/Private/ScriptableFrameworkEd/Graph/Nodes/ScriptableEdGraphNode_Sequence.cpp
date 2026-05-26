// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/Nodes/ScriptableEdGraphNode_Sequence.h"
#include "ScriptableFrameworkEd/Graph/ScriptableGraphCommands.h"
#include "ScriptableNodes/ScriptableNode_Sequence.h"

#include "EdGraph/EdGraphPin.h"
#include "ToolMenu.h"
#include "ToolMenuSection.h"

#define LOCTEXT_NAMESPACE "ScriptableEdGraphNode_Sequence"

UScriptableEdGraphNode_Sequence::UScriptableEdGraphNode_Sequence()
{
	RuntimeNodeClass = UScriptableNode_Sequence::StaticClass();
}

FSlateIcon UScriptableEdGraphNode_Sequence::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = FLinearColor::White;
	return FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.Sequence_16x");
}

void UScriptableEdGraphNode_Sequence::AppendPinContextActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	if (!Menu || !Context || !Context->Pin) return;

	// Only output pins (the "Then N" branches) are removable.
	if (Context->Pin->Direction != EGPD_Output) return;

	// Sanity check: don't expose Remove on a node that isn't actually a Sequence.
	const UScriptableNode_Sequence* SequenceRuntime = Cast<UScriptableNode_Sequence>(GetRuntimeNode());
	if (!SequenceRuntime) return;

	FToolMenuSection& Section = Menu->AddSection("ScriptableSequencePinActions", LOCTEXT("PinActionsHeader", "Sequence"));
	Section.AddMenuEntry(FScriptableGraphCommands::Get().RemoveSequencePin);
}

#undef LOCTEXT_NAMESPACE
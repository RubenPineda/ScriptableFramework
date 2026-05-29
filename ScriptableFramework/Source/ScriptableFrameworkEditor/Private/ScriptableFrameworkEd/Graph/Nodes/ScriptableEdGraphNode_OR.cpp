// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/Nodes/ScriptableEdGraphNode_OR.h"
#include "ScriptableFrameworkEd/Graph/ScriptableGraphCommands.h"
#include "ScriptableNodes/ScriptableNode_OR.h"

#include "EdGraph/EdGraphPin.h"
#include "ToolMenu.h"
#include "ToolMenuSection.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "ScriptableEdGraphNode_OR"

UScriptableEdGraphNode_OR::UScriptableEdGraphNode_OR()
{
	RuntimeNodeClass = UScriptableNode_OR::StaticClass();
}

FSlateIcon UScriptableEdGraphNode_OR::GetIconAndTint(FLinearColor& OutColor) const
{
	// Reuse the same compact macro glyph AND uses; visually the two nodes belong to the same family.
	OutColor = FLinearColor::White;
	return FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.Macro.Loop_16x");
}

void UScriptableEdGraphNode_OR::AppendPinContextActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	if (!Menu || !Context || !Context->Pin) return;

	// Only input pins are removable; the sole "Out" output is permanent.
	if (Context->Pin->Direction != EGPD_Input) return;

	if (!Cast<UScriptableNode_OR>(GetRuntimeNode())) return;

	FToolMenuSection& Section = Menu->AddSection("ScriptableORPinActions", LOCTEXT("PinActionsHeader", "OR"));
	Section.AddMenuEntry(FScriptableGraphCommands::Get().RemoveORPin);
}

#undef LOCTEXT_NAMESPACE

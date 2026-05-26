// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode_AND.h"
#include "ScriptableFrameworkEd/Graph/ScriptableGraphCommands.h"
#include "ScriptableNodes/ScriptableNode_AND.h"

#include "EdGraph/EdGraphPin.h"
#include "ToolMenu.h"
#include "ToolMenuSection.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "ScriptableEdGraphNode_AND"

UScriptableEdGraphNode_AND::UScriptableEdGraphNode_AND()
{
	RuntimeNodeClass = UScriptableNode_AND::StaticClass();
}

FSlateIcon UScriptableEdGraphNode_AND::GetIconAndTint(FLinearColor& OutColor) const
{
	// No first-party AND glyph in FAppStyle; reuse the math-compact glyph used by BP's logical
	// operator nodes. If a more specific brush surfaces later we can swap it without touching the
	// rest of the visual stack.
	OutColor = FLinearColor::White;
	return FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.Macro.Loop_16x");
}

void UScriptableEdGraphNode_AND::AppendPinContextActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	if (!Menu || !Context || !Context->Pin) return;

	// Only input pins are removable. The sole "Out" output is permanent.
	if (Context->Pin->Direction != EGPD_Input) return;

	if (!Cast<UScriptableNode_AND>(GetRuntimeNode())) return;

	FToolMenuSection& Section = Menu->AddSection("ScriptableANDPinActions", LOCTEXT("PinActionsHeader", "AND"));
	Section.AddMenuEntry(FScriptableGraphCommands::Get().RemoveANDPin);
}

#undef LOCTEXT_NAMESPACE
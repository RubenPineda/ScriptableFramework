// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/ScriptableGraphCommands.h"
#include "Framework/Commands/InputChord.h"

#define LOCTEXT_NAMESPACE "ScriptableGraphCommands"

void FScriptableGraphCommands::RegisterCommands()
{
	UI_COMMAND(
		RemoveSequencePin,
		"Remove pin",
		"Remove this branch from the Sequence node. Higher-indexed branches shift down to fill the gap and keep their downstream connections.",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		RemoveANDPin,
		"Remove pin",
		"Remove this input from the AND node. Higher-indexed inputs shift down to fill the gap and keep their incoming connections.",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		RemoveORPin,
		"Remove pin",
		"Remove this input from the OR node. Higher-indexed inputs shift down to fill the gap and keep their incoming connections.",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(AlignNodesTop,    "Align Top",    "Align selected nodes to the topmost node.",    EUserInterfaceActionType::Button, FInputChord(EModifierKey::Shift, EKeys::W));
	UI_COMMAND(AlignNodesBottom, "Align Bottom", "Align selected nodes to the bottommost node.", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Shift, EKeys::S));
	UI_COMMAND(AlignNodesLeft,   "Align Left",   "Align selected nodes to the leftmost node.",   EUserInterfaceActionType::Button, FInputChord(EModifierKey::Shift, EKeys::A));
	UI_COMMAND(AlignNodesRight,  "Align Right",  "Align selected nodes to the rightmost node.",  EUserInterfaceActionType::Button, FInputChord(EModifierKey::Shift, EKeys::D));
	UI_COMMAND(AlignNodesMiddle, "Align Middle", "Align selected nodes to the vertical centerline of the selection.",   EUserInterfaceActionType::Button, FInputChord(EModifierKey::Shift, EKeys::M));
	UI_COMMAND(AlignNodesCenter, "Align Center", "Align selected nodes to the horizontal centerline of the selection.", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Shift, EKeys::C));
	UI_COMMAND(DistributeNodesHorizontally, "Distribute Horizontally", "Evenly distribute selected nodes along the X axis between the leftmost and rightmost.", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Shift, EKeys::H));
	UI_COMMAND(DistributeNodesVertically,   "Distribute Vertically",   "Evenly distribute selected nodes along the Y axis between the topmost and bottommost.",  EUserInterfaceActionType::Button, FInputChord(EModifierKey::Shift, EKeys::V));
	UI_COMMAND(ZoomToSelection,  "Zoom to Selection", "Pan and zoom the graph view to fit the current selection.", EUserInterfaceActionType::Button, FInputChord(EKeys::F));
	UI_COMMAND(OpenSearch, "Find in Graph", "Open the Search panel and focus its search box.", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::F));
	UI_COMMAND(ToggleBreakpoint, "Toggle Breakpoint", "Add or remove a breakpoint on the selected node(s).", EUserInterfaceActionType::Button, FInputChord(EKeys::F9));
	UI_COMMAND(ConvertSelectionToSubGraph, "Convert to Sub-Graph", "Move the selected nodes into a new Scriptable Graph asset and replace them with a SubGraph node referencing it. Inbound wires become event inputs, outbound wires become outputs.", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
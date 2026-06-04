// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "Styling/AppStyle.h"

/** Centralized FUICommandInfo registry for the Scriptable Graph editor. */
class SCRIPTABLEFRAMEWORKEDITOR_API FScriptableGraphCommands : public TCommands<FScriptableGraphCommands>
{
public:
	FScriptableGraphCommands()
		: TCommands<FScriptableGraphCommands>(
			TEXT("ScriptableGraphEditor"),
			NSLOCTEXT("Contexts", "ScriptableGraphEditor", "Scriptable Graph Editor"),
			NAME_None,
			FAppStyle::GetAppStyleSetName())
	{
	}

	virtual void RegisterCommands() override;

	/** Removes the right-clicked output pin from a Sequence node, sliding higher branches down. */
	TSharedPtr<FUICommandInfo> RemoveSequencePin;

	/** Removes the right-clicked input pin from an AND node, sliding higher inputs down. */
	TSharedPtr<FUICommandInfo> RemoveANDPin;

	/** Removes the right-clicked input pin from an OR node, sliding higher inputs down. */
	TSharedPtr<FUICommandInfo> RemoveORPin;

	/** Cardinal alignment of selected nodes (Shift+W/S/A/D). */
	TSharedPtr<FUICommandInfo> AlignNodesTop;
	TSharedPtr<FUICommandInfo> AlignNodesBottom;
	TSharedPtr<FUICommandInfo> AlignNodesLeft;
	TSharedPtr<FUICommandInfo> AlignNodesRight;

	/** Centerline alignment of selected nodes (Shift+M vertical, Shift+C horizontal). */
	TSharedPtr<FUICommandInfo> AlignNodesMiddle;
	TSharedPtr<FUICommandInfo> AlignNodesCenter;

	/** Even spacing along an axis (Shift+H horizontal, Shift+V vertical). */
	TSharedPtr<FUICommandInfo> DistributeNodesHorizontally;
	TSharedPtr<FUICommandInfo> DistributeNodesVertically;

	/** Zoom and pan the graph view to fit the current selection (F). */
	TSharedPtr<FUICommandInfo> ZoomToSelection;

	/** Toggle breakpoint on the selected node(s) (F9). BP behaviour: add when missing, remove when present. */
	TSharedPtr<FUICommandInfo> ToggleBreakpoint;
};
// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode_Native.h"
#include "ScriptableEdGraphNode_Sequence.generated.h"

class UToolMenu;
class UGraphNodeContextMenuContext;

/**
 * Specialized visual node for UScriptableNode_Sequence. Inherits the generic native styling
 * (system tint, unlabelled sole input) and contributes pin-level context-menu actions:
 * specifically, "Remove pin" on each output, which preserves downstream connections by sliding
 * higher-indexed branches down to fill the gap.
 */
UCLASS()
class UScriptableEdGraphNode_Sequence : public UScriptableEdGraphNode_Native
{
	GENERATED_BODY()

public:
	UScriptableEdGraphNode_Sequence();

	//~ UEdGraphNode interface
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual bool ShowPaletteIconOnNode() const override { return true; }
	//~ End of UEdGraphNode interface

	//~ UScriptableEdGraphNode interface
	virtual void AppendPinContextActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;
	//~ End of UScriptableEdGraphNode interface
};
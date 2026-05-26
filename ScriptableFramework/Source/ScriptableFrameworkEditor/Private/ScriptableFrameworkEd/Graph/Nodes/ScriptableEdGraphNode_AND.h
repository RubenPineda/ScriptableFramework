// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode_Native.h"
#include "ScriptableEdGraphNode_AND.generated.h"

/**
 * Specialized ed-node for UScriptableNode_AND. Adds per-input "Remove pin" context actions —
 * Sequence's per-output affordance, mirrored onto the input side.
 */
UCLASS()
class UScriptableEdGraphNode_AND : public UScriptableEdGraphNode_Native
{
	GENERATED_BODY()

public:
	UScriptableEdGraphNode_AND();

	//~ UEdGraphNode interface
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual bool ShowPaletteIconOnNode() const override { return true; }
	//~ End of UEdGraphNode interface

	//~ UScriptableEdGraphNode interface
	virtual void AppendPinContextActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;
	//~ End of UScriptableEdGraphNode interface
};
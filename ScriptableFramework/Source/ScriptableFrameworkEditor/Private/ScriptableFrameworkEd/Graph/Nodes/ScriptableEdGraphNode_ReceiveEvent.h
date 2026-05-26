// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode_Native.h"
#include "ScriptableEdGraphNode_ReceiveEvent.generated.h"

/**
 * Specialized ed-node for UScriptableNode_ReceiveEvent. Reuses Entry's red tint (both are
 * origin-of-flow) and the event icon; the canvas title comes from the runtime's GetDisplayTitle.
 */
UCLASS()
class UScriptableEdGraphNode_ReceiveEvent : public UScriptableEdGraphNode_Native
{
	GENERATED_BODY()

public:
	UScriptableEdGraphNode_ReceiveEvent();

	//~ UEdGraphNode interface
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual bool ShowPaletteIconOnNode() const override { return true; }
	//~ End of UEdGraphNode interface
};
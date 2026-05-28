// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode_Native.h"
#include "ScriptableEdGraphNode_Exit.generated.h"

/** Specialized ed-node for UScriptableNode_Exit: system tint and a conduit icon. */
UCLASS()
class UScriptableEdGraphNode_Exit : public UScriptableEdGraphNode_Native
{
	GENERATED_BODY()

public:
	UScriptableEdGraphNode_Exit();

	//~ UEdGraphNode interface
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual bool ShowPaletteIconOnNode() const override { return true; }

	/** Exit is unique per graph: keep copy/paste/duplicate from making a second one. */
	virtual bool CanDuplicateNode() const override { return false; }
	//~ End of UEdGraphNode interface
};

// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode_Native.h"
#include "ScriptableEdGraphNode_Branch.generated.h"

/** Specialized ed-node for UScriptableNode_Branch. */
UCLASS()
class UScriptableEdGraphNode_Branch : public UScriptableEdGraphNode_Native
{
	GENERATED_BODY()

public:
	//~ UEdGraphNode interface
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual bool ShowPaletteIconOnNode() const override { return true; }
	//~ End of UEdGraphNode interface

	//~ UScriptableEdGraphNode interface
	virtual bool ShouldShowPinLabel(FName PinName) const override;
	//~ End of UScriptableEdGraphNode interface
};
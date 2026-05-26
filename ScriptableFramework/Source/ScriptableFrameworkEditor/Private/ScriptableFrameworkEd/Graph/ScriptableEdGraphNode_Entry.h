// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode.h"
#include "ScriptableEdGraphNode_Entry.generated.h"

/**
 * Visual node representing the graph's Entry runtime node.
 */
UCLASS()
class UScriptableEdGraphNode_Entry : public UScriptableEdGraphNode
{
	GENERATED_BODY()

public:
	UScriptableEdGraphNode_Entry();

	//~ UEdGraphNode interface
	virtual bool CanUserDeleteNode() const override { return false; }
	virtual bool CanDuplicateNode() const override { return false; }
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual bool ShowPaletteIconOnNode() const override { return true; }
	//~ End of UEdGraphNode interface

	virtual bool ShouldShowPinLabel(FName PinName) const override { return false; }
};
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
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual bool ShowPaletteIconOnNode() const override { return true; }

	/** F2 / inline rename edits the event's name directly on the node. */
	virtual bool GetCanRenameNode() const override { return true; }
	virtual void OnRenameNode(const FString& NewName) override;
	//~ End of UEdGraphNode interface
};
// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphSchema.h"
#include "ScriptableEdGraphSchema.generated.h"

/**
 * Schema for the scriptable graph editor. Defines which pins can connect, what the
 * context menu offers, and other editor-only graph semantics.
 */
UCLASS()
class UScriptableEdGraphSchema : public UEdGraphSchema
{
	GENERATED_BODY()

public:
	//~ UEdGraphSchema interface
	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const override;
	virtual bool TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const override;
	virtual void BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const override;
	virtual void BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotification) const override;
	virtual void BreakNodeLinks(UEdGraphNode& TargetNode) const override;
	virtual void GetContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;
	virtual class FConnectionDrawingPolicy* CreateConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const override;
	//~ End of UEdGraphSchema interface

private:
	/** Persists a new wire into the owning UScriptableGraph::Connections list. */
	void PersistConnection(UEdGraphPin* PinA, UEdGraphPin* PinB) const;
};
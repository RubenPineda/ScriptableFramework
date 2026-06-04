// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ConnectionDrawingPolicy.h"

class UEdGraph;

/**
 * Connection drawing policy: like the default but suppresses the mid-wire arrowhead — direction
 * is already implied by the BP-style exec triangles at each pin. Also reads the host graph's
 * SelectedNodes to brighten wires incident to a selected node and reads pin bOrphanedPin to
 * tint dangling wires red.
 */
class FScriptableConnectionDrawingPolicy : public FConnectionDrawingPolicy
{
public:
	FScriptableConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj);

	//~ FConnectionDrawingPolicy interface
	virtual void DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, FConnectionParams& Params) override;
	//~ End of FConnectionDrawingPolicy interface

private:
	UEdGraph* GraphObj;
};
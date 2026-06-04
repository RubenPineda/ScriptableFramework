// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ConnectionDrawingPolicy.h"

/**
 * Connection drawing policy: like the default but suppresses the mid-wire arrowhead — direction
 * is already implied by the BP-style exec triangles at each pin.
 */
class FScriptableConnectionDrawingPolicy : public FConnectionDrawingPolicy
{
public:
	FScriptableConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements);

	//~ FConnectionDrawingPolicy interface
	virtual void DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, FConnectionParams& Params) override;
	//~ End of FConnectionDrawingPolicy interface
};
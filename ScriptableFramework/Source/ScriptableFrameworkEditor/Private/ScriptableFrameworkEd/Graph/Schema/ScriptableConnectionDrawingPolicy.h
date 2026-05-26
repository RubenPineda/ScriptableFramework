// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ConnectionDrawingPolicy.h"

/**
 * Connection drawing policy for the scriptable graph. Mirrors the default behaviour but
 * suppresses the mid-wire arrow head — wire direction is already implied by the BP-style
 * exec triangles at each pin.
 */
class FScriptableConnectionDrawingPolicy : public FConnectionDrawingPolicy
{
public:
	FScriptableConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements);
};
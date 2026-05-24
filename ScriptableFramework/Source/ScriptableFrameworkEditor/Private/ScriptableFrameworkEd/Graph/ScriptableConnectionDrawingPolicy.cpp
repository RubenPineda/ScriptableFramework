// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/ScriptableConnectionDrawingPolicy.h"

FScriptableConnectionDrawingPolicy::FScriptableConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements)
	: FConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements)
{
	// Suppress the mid-wire arrow head. Direction is conveyed by the exec triangles at the pins.
	ArrowImage = nullptr;
	ArrowRadius = FVector2D::ZeroVector;
}
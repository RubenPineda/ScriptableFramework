// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/Schema/ScriptableConnectionDrawingPolicy.h"

FScriptableConnectionDrawingPolicy::FScriptableConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements)
	: FConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements)
{
	// Suppress the mid-wire arrow head. Direction is conveyed by the exec triangles at the pins.
	ArrowImage = nullptr;
	ArrowRadius = FVector2D::ZeroVector;
}

void FScriptableConnectionDrawingPolicy::DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, FConnectionParams& Params)
{
	FConnectionDrawingPolicy::DetermineWiringStyle(OutputPin, InputPin, Params);

	/** Match Blueprint: any wire touching an orphan pin renders red so the dangling reference is obvious. */
	const bool bOrphan = (OutputPin && OutputPin->bOrphanedPin) || (InputPin && InputPin->bOrphanedPin);
	if (bOrphan)
	{
		Params.WireColor = FLinearColor::Red;
	}
}
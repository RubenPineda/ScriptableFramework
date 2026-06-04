// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/Schema/ScriptableConnectionDrawingPolicy.h"
#include "ScriptableFrameworkEd/Graph/Schema/ScriptableEdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphNode.h"

FScriptableConnectionDrawingPolicy::FScriptableConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj)
	: FConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements)
	, GraphObj(InGraphObj)
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
		return;
	}

	/** Brighten + thicken wires incident to any selected node, mirroring the BP "selection-relationship" cue. */
	if (const UScriptableEdGraph* SfGraph = Cast<UScriptableEdGraph>(GraphObj))
	{
		const UEdGraphNode* FromNode = OutputPin ? OutputPin->GetOwningNode() : nullptr;
		const UEdGraphNode* ToNode = InputPin ? InputPin->GetOwningNode() : nullptr;
		const bool bTouchesSelection = (FromNode && SfGraph->SelectedNodes.Contains(FromNode))
			|| (ToNode && SfGraph->SelectedNodes.Contains(ToNode));
		if (bTouchesSelection)
		{
			Params.WireThickness *= 1.75f;
			Params.WireColor *= 1.4f;
		}
	}
}
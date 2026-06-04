// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/Widgets/SScriptableGraphNode_Base.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode.h"
#include "ScriptableNodes/ScriptableGraph.h"
#include "ScriptableNodes/ScriptableNode.h"

#include "Styling/AppStyle.h"

void SScriptableGraphNode_Base::Construct(const FArguments& InArgs, UEdGraphNode* InNode)
{
	GraphNode = InNode;
	SetCursor(EMouseCursor::CardinalCross);
	UpdateGraphNode();
}

void SScriptableGraphNode_Base::GetOverlayBrushes(bool bSelected, const FVector2f& WidgetSize, TArray<FOverlayBrushInfo>& Brushes) const
{
	SGraphNode::GetOverlayBrushes(bSelected, WidgetSize, Brushes);

	const UScriptableEdGraphNode* SfNode = Cast<UScriptableEdGraphNode>(GraphNode);
	if (!SfNode || !SfNode->GetRuntimeNode()) return;

	const UScriptableGraph* Asset = SfNode->GetTypedOuter<UScriptableGraph>();
	if (!Asset) return;
	const bool* EnabledPtr = Asset->Breakpoints.Find(SfNode->GetRuntimeNode()->GetBindingID());
	if (!EnabledPtr) return;

	/** Filled red dot when enabled, hollow when disabled — mirroring SGraphNodeK2Base. Centred top-left. */
	FOverlayBrushInfo BreakpointInfo;
	BreakpointInfo.Brush = FAppStyle::GetBrush(*EnabledPtr
		? TEXT("Kismet.DebuggerOverlay.Breakpoint.EnabledAndValid")
		: TEXT("Kismet.DebuggerOverlay.Breakpoint.Disabled"));
	if (BreakpointInfo.Brush)
	{
		BreakpointInfo.OverlayOffset -= BreakpointInfo.Brush->ImageSize / 2.f;
		Brushes.Add(BreakpointInfo);
	}
}

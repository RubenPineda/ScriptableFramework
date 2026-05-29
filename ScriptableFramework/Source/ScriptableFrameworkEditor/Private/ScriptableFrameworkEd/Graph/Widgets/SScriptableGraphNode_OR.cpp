// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/Widgets/SScriptableGraphNode_OR.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode.h"
#include "ScriptableNodes/ScriptableNode_OR.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "GraphEditorSettings.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "SScriptableGraphNode_OR"

void SScriptableGraphNode_OR::Construct(const FArguments& InArgs, UScriptableEdGraphNode* InNode)
{
	GraphNode = InNode;
	SetCursor(EMouseCursor::CardinalCross);
	UpdateGraphNode();
}

void SScriptableGraphNode_OR::CreateInputSideAddButton(TSharedPtr<SVerticalBox> InputBox)
{
	TSharedRef<SWidget> AddPinButton = AddPinButtonContent(
		LOCTEXT("AddPinLabel", "Add pin"),
		LOCTEXT("AddPinTooltip", "Add new input pin"));

	FMargin AddPinPadding = Settings->GetInputPinPadding();
	AddPinPadding.Top += 6.0f;

	InputBox->AddSlot()
		.AutoHeight()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Left)
		.Padding(AddPinPadding)
		[
			AddPinButton
		];
}

FReply SScriptableGraphNode_OR::OnAddPin()
{
	UScriptableNode_OR* OR = GetRuntimeOR();
	if (!OR || !GraphNode) return FReply::Handled();

	// AddInputPin only bumps the runtime counter; the ed-node's Pins array is unchanged.
	// ReconstructNode re-runs AllocateDefaultPins (which queries the runtime's GetInputPins) so the
	// new pin lands on the ed-node, then NotifyGraphChanged inside ReconstructNode triggers SGraphPanel
	// to redraw. Same fix as the AND widget.
	const FScopedTransaction Transaction(LOCTEXT("AddORPin", "Add OR Pin"));
	OR->AddInputPin();
	GraphNode->ReconstructNode();
	UpdateGraphNode();

	return FReply::Handled();
}

EVisibility SScriptableGraphNode_OR::IsAddPinButtonVisible() const
{
	// Always available — OR has no upper bound on input count.
	return EVisibility::Visible;
}

UScriptableNode_OR* SScriptableGraphNode_OR::GetRuntimeOR() const
{
	UScriptableEdGraphNode* EdNode = Cast<UScriptableEdGraphNode>(GraphNode);
	if (!EdNode) return nullptr;
	return Cast<UScriptableNode_OR>(EdNode->GetRuntimeNode());
}

#undef LOCTEXT_NAMESPACE

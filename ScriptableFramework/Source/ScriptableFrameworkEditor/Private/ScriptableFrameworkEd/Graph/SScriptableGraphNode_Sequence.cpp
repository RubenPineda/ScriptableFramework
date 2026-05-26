// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/SScriptableGraphNode_Sequence.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode.h"
#include "ScriptableNodes/ScriptableNode_Sequence.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "GraphEditorSettings.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "SScriptableGraphNode_Sequence"

void SScriptableGraphNode_Sequence::Construct(const FArguments& InArgs, UScriptableEdGraphNode* InNode)
{
	GraphNode = InNode;
	SetCursor(EMouseCursor::CardinalCross);
	UpdateGraphNode();
}

void SScriptableGraphNode_Sequence::CreateOutputSideAddButton(TSharedPtr<SVerticalBox> OutputBox)
{
	// Match the K2 sequence node visual exactly. SGraphNode::AddPinButtonContent is the canonical
	// helper that builds an "Add pin" affordance with the standard label font, icon, hover state
	// and spacing — same construction K2's SGraphNodeK2Sequence uses. Padding comes from the user's
	// graph editor settings (Settings->GetOutputPinPadding) so the button aligns with the pin
	// column above it for any zoom level or DPI setting.
	TSharedRef<SWidget> AddPinButton = AddPinButtonContent(
		LOCTEXT("AddPinLabel", "Add pin"),
		LOCTEXT("AddPinTooltip", "Add new pin"));

	FMargin AddPinPadding = Settings->GetOutputPinPadding();
	AddPinPadding.Top += 6.0f;

	OutputBox->AddSlot()
		.AutoHeight()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Right)
		.Padding(AddPinPadding)
		[
			AddPinButton
		];
}

FReply SScriptableGraphNode_Sequence::OnAddPin()
{
	UScriptableNode_Sequence* Sequence = GetRuntimeSequence();
	if (!Sequence) return FReply::Handled();

	// SGraphNode invokes this via the AddPin button helper above. Mirror the K2 path: scoped
	// transaction for undo, mutate the model, refresh the slate node, notify the graph.
	const FScopedTransaction Transaction(LOCTEXT("AddSequencePin", "Add Sequence Pin"));
	Sequence->AddOutputPin();
	UpdateGraphNode();
	if (UEdGraph* OwningGraph = GraphNode ? GraphNode->GetGraph() : nullptr)
	{
		OwningGraph->NotifyNodeChanged(GraphNode);
	}

	return FReply::Handled();
}

EVisibility SScriptableGraphNode_Sequence::IsAddPinButtonVisible() const
{
	// Always available — Sequence has no upper bound on output count.
	return EVisibility::Visible;
}

UScriptableNode_Sequence* SScriptableGraphNode_Sequence::GetRuntimeSequence() const
{
	UScriptableEdGraphNode* EdNode = Cast<UScriptableEdGraphNode>(GraphNode);
	if (!EdNode) return nullptr;
	return Cast<UScriptableNode_Sequence>(EdNode->GetRuntimeNode());
}

#undef LOCTEXT_NAMESPACE
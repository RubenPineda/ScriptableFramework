// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/Widgets/SScriptableGraphNode_AND.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode.h"
#include "ScriptableNodes/ScriptableNode_AND.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "GraphEditorSettings.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "SScriptableGraphNode_AND"

void SScriptableGraphNode_AND::Construct(const FArguments& InArgs, UScriptableEdGraphNode* InNode)
{
	GraphNode = InNode;
	SetCursor(EMouseCursor::CardinalCross);
	UpdateGraphNode();
}

void SScriptableGraphNode_AND::CreateInputSideAddButton(TSharedPtr<SVerticalBox> InputBox)
{
	// Same construction as Sequence's CreateOutputSideAddButton but routed to the input column.
	// SGraphNode::AddPinButtonContent is the canonical helper that builds an "Add pin" affordance
	// with the standard label font and icon; padding comes from the user's graph editor settings
	// so the button aligns with the pin column above it.
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

FReply SScriptableGraphNode_AND::OnAddPin()
{
	UScriptableNode_AND* AND = GetRuntimeAND();
	if (!AND) return FReply::Handled();

	const FScopedTransaction Transaction(LOCTEXT("AddANDPin", "Add AND Pin"));
	AND->AddInputPin();
	UpdateGraphNode();
	if (UEdGraph* OwningGraph = GraphNode ? GraphNode->GetGraph() : nullptr)
	{
		OwningGraph->NotifyNodeChanged(GraphNode);
	}

	return FReply::Handled();
}

EVisibility SScriptableGraphNode_AND::IsAddPinButtonVisible() const
{
	// Always available — AND has no upper bound on input count.
	return EVisibility::Visible;
}

UScriptableNode_AND* SScriptableGraphNode_AND::GetRuntimeAND() const
{
	UScriptableEdGraphNode* EdNode = Cast<UScriptableEdGraphNode>(GraphNode);
	if (!EdNode) return nullptr;
	return Cast<UScriptableNode_AND>(EdNode->GetRuntimeNode());
}

#undef LOCTEXT_NAMESPACE
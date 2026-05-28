// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/Widgets/SScriptableGraphNode_GoTo.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode.h"
#include "ScriptableNodes/ScriptableNode_GoTo.h"
#include "ScriptableNodes/ScriptableNode_ReceiveEvent.h"
#include "ScriptableNodes/ScriptableGraph.h"

#include "EdGraph/EdGraph.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ScopedTransaction.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SScriptableGraphNode_GoTo"

void SScriptableGraphNode_GoTo::Construct(const FArguments& InArgs, UScriptableEdGraphNode* InNode)
{
	GraphNode = InNode;
	SetCursor(EMouseCursor::CardinalCross);
	UpdateGraphNode();
}

UScriptableNode_GoTo* SScriptableGraphNode_GoTo::GetRuntimeGoTo() const
{
	UScriptableEdGraphNode* EdNode = Cast<UScriptableEdGraphNode>(GraphNode);
	return EdNode ? Cast<UScriptableNode_GoTo>(EdNode->GetRuntimeNode()) : nullptr;
}

void SScriptableGraphNode_GoTo::CreateBelowPinControls(TSharedPtr<SVerticalBox> MainBox)
{
	if (!MainBox.IsValid()) return;

	MainBox->AddSlot()
		.AutoHeight()
		.Padding(8.f, 2.f, 8.f, 6.f)
		[
			SNew(SComboButton)
				.OnGetMenuContent(this, &SScriptableGraphNode_GoTo::BuildEventMenu)
				.ButtonContent()
				[
					SNew(STextBlock)
						.Text(this, &SScriptableGraphNode_GoTo::GetSelectedEventText)
				]
		];
}

FText SScriptableGraphNode_GoTo::GetSelectedEventText() const
{
	const UScriptableNode_GoTo* GoTo = GetRuntimeGoTo();
	if (GoTo && !GoTo->TargetEvent.IsNone())
	{
		return FText::FromName(GoTo->TargetEvent);
	}
	return LOCTEXT("SelectEvent", "Select event...");
}

TSharedRef<SWidget> SScriptableGraphNode_GoTo::BuildEventMenu()
{
	FMenuBuilder MenuBuilder(/*bShouldCloseWindowAfterMenuSelection*/ true, nullptr);

	const UScriptableNode_GoTo* GoTo = GetRuntimeGoTo();
	const UScriptableGraph* Graph = GoTo ? GoTo->GetTypedOuter<UScriptableGraph>() : nullptr;

	if (Graph)
	{
		// Collect the distinct, named ReceiveEvent keys declared anywhere in the graph.
		TArray<FName> EventNames;
		for (const TObjectPtr<UScriptableNode>& Node : Graph->Nodes)
		{
			const UScriptableNode_ReceiveEvent* Receiver = Cast<UScriptableNode_ReceiveEvent>(Node);
			if (Receiver && !Receiver->EventName.IsNone())
			{
				EventNames.AddUnique(Receiver->EventName);
			}
		}
		EventNames.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });

		for (const FName& EventName : EventNames)
		{
			MenuBuilder.AddMenuEntry(
				FText::FromName(EventName),
				FText::GetEmpty(),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SScriptableGraphNode_GoTo::OnEventPicked, EventName)));
		}

		if (EventNames.IsEmpty())
		{
			MenuBuilder.AddWidget(SNew(STextBlock).Text(LOCTEXT("NoEvents", "No event nodes in this graph")), FText::GetEmpty());
		}
	}

	return MenuBuilder.MakeWidget();
}

void SScriptableGraphNode_GoTo::OnEventPicked(FName EventName)
{
	UScriptableNode_GoTo* GoTo = GetRuntimeGoTo();
	if (!GoTo) return;

	const FScopedTransaction Transaction(LOCTEXT("SetGoToEvent", "Set Go To Event"));
	GoTo->Modify();
	GoTo->TargetEvent = EventName;

	// Refresh the node title ("Go to X") and rebuild the combo label.
	if (UEdGraph* OwningGraph = GraphNode ? GraphNode->GetGraph() : nullptr)
	{
		OwningGraph->NotifyNodeChanged(GraphNode);
	}
	UpdateGraphNode();
}

#undef LOCTEXT_NAMESPACE

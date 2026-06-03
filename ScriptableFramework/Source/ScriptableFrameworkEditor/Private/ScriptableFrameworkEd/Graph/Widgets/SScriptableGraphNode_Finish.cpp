// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/Widgets/SScriptableGraphNode_Finish.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode.h"
#include "ScriptableNodes/ScriptableNode_Finish.h"
#include "ScriptableNodes/ScriptableGraph.h"

#include "EdGraph/EdGraph.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ScopedTransaction.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SScriptableGraphNode_Finish"

void SScriptableGraphNode_Finish::Construct(const FArguments& InArgs, UScriptableEdGraphNode* InNode)
{
	GraphNode = InNode;
	SetCursor(EMouseCursor::CardinalCross);
	UpdateGraphNode();
}

UScriptableNode_Finish* SScriptableGraphNode_Finish::GetRuntimeFinish() const
{
	UScriptableEdGraphNode* EdNode = Cast<UScriptableEdGraphNode>(GraphNode);
	return EdNode ? Cast<UScriptableNode_Finish>(EdNode->GetRuntimeNode()) : nullptr;
}

void SScriptableGraphNode_Finish::CreateBelowPinControls(TSharedPtr<SVerticalBox> MainBox)
{
	if (!MainBox.IsValid()) return;

	MainBox->AddSlot()
		.AutoHeight()
		.Padding(8.f, 2.f, 8.f, 6.f)
		[
			SNew(SComboButton)
				.Visibility(this, &SScriptableGraphNode_Finish::GetComboVisibility)
				.OnGetMenuContent(this, &SScriptableGraphNode_Finish::BuildOutputMenu)
				.ButtonContent()
				[
					SNew(STextBlock)
						.Text(this, &SScriptableGraphNode_Finish::GetSelectedOutputText)
				]
		];
}

EVisibility SScriptableGraphNode_Finish::GetComboVisibility() const
{
	const UScriptableNode_Finish* Finish = GetRuntimeFinish();
	if (!Finish) return EVisibility::Collapsed;

	/** Already has a non-None pick: surface the combo so the user can change or clear it. */
	if (!Finish->OutputName.IsNone()) return EVisibility::Visible;

	const UScriptableGraph* Graph = Finish->GetTypedOuter<UScriptableGraph>();
	if (!Graph) return EVisibility::Collapsed;

	for (const FName& Output : Graph->Outputs)
	{
		if (!Output.IsNone()) return EVisibility::Visible;
	}
	return EVisibility::Collapsed;
}

FText SScriptableGraphNode_Finish::GetSelectedOutputText() const
{
	const UScriptableNode_Finish* Finish = GetRuntimeFinish();
	if (Finish && !Finish->OutputName.IsNone())
	{
		return FText::FromName(Finish->OutputName);
	}
	return LOCTEXT("SelectOutput", "Select output...");
}

TSharedRef<SWidget> SScriptableGraphNode_Finish::BuildOutputMenu()
{
	FMenuBuilder MenuBuilder(/*bShouldCloseWindowAfterMenuSelection*/ true, nullptr);

	const UScriptableNode_Finish* Finish = GetRuntimeFinish();
	const UScriptableGraph* Graph = Finish ? Finish->GetTypedOuter<UScriptableGraph>() : nullptr;

	/** Clear-to-default entry: persists OutputName=None so the runtime falls back to Exit's "Finished". */
	MenuBuilder.AddMenuEntry(
		LOCTEXT("DefaultFinished", "Default (Finished)"),
		LOCTEXT("DefaultFinishedTip", "Clear the pick; the runtime falls back to Exit's 'Finished' output."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &SScriptableGraphNode_Finish::OnOutputPicked, FName(NAME_None))));

	if (Graph)
	{
		const bool bHasUserOutputs = Graph->Outputs.ContainsByPredicate([](const FName& Out) { return !Out.IsNone(); });
		if (bHasUserOutputs) MenuBuilder.AddMenuSeparator();

		// Asset-declared Outputs only. The Exit built-in "Cancelled" semantically belongs to
		// external Cancel(), not to a deliberate terminator, so it's not a picker target.
		for (const FName& Output : Graph->Outputs)
		{
			if (Output.IsNone()) continue;
			MenuBuilder.AddMenuEntry(
				FText::FromName(Output),
				FText::GetEmpty(),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SScriptableGraphNode_Finish::OnOutputPicked, Output)));
		}
	}

	return MenuBuilder.MakeWidget();
}

void SScriptableGraphNode_Finish::OnOutputPicked(FName OutputName)
{
	UScriptableNode_Finish* Finish = GetRuntimeFinish();
	if (!Finish) return;

	const FScopedTransaction Transaction(LOCTEXT("SetFinishOutput", "Set Finish Output"));
	Finish->Modify();
	Finish->OutputName = OutputName;

	if (UEdGraph* OwningGraph = GraphNode ? GraphNode->GetGraph() : nullptr)
	{
		OwningGraph->NotifyNodeChanged(GraphNode);
	}
	UpdateGraphNode();
}

#undef LOCTEXT_NAMESPACE

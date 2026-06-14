// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/Diff/SScriptableGraphDiff.h"
#include "ScriptableFrameworkEd/Graph/Editor/ScriptableGraphEditor.h"
#include "ScriptableNodes/ScriptableGraph.h"

#include "GraphEditor.h"
#include "EdGraph/EdGraph.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SWindow.h"

#define LOCTEXT_NAMESPACE "SScriptableGraphDiff"

void SScriptableGraphDiff::CreateDiffWindow(const UScriptableGraph* OldGraph, const UScriptableGraph* NewGraph, const FRevisionInfo& OldRevision, const FRevisionInfo& NewRevision)
{
	const TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("DiffWindowTitle", "Scriptable Graph Diff"))
		.ClientSize(FVector2D(1400.0f, 800.0f));

	Window->SetContent(
		SNew(SScriptableGraphDiff)
			.OldGraph(OldGraph)
			.NewGraph(NewGraph)
			.OldRevision(OldRevision)
			.NewRevision(NewRevision));

	FSlateApplication::Get().AddWindow(Window);
}

FText SScriptableGraphDiff::MakeRevisionLabel(const FRevisionInfo& Revision, const FText& Fallback)
{
	if (Revision.Revision.IsEmpty())
	{
		return Fallback;
	}
	return FText::Format(LOCTEXT("RevisionLabelFmt", "Revision {0}"), FText::FromString(Revision.Revision));
}

void SScriptableGraphDiff::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SSplitter)
			+ SSplitter::Slot()
			.Value(0.5f)
			[
				BuildGraphPanel(InArgs._OldGraph, MakeRevisionLabel(InArgs._OldRevision, LOCTEXT("OldRevision", "Old Revision")), OldGraphEditor)
			]
			+ SSplitter::Slot()
			.Value(0.5f)
			[
				BuildGraphPanel(InArgs._NewGraph, MakeRevisionLabel(InArgs._NewRevision, LOCTEXT("NewRevision", "New / Local")), NewGraphEditor)
			]
	];
}

TSharedRef<SWidget> SScriptableGraphDiff::BuildGraphPanel(const UScriptableGraph* Graph, const FText& Title, TSharedPtr<SGraphEditor>& OutEditor)
{
	TSharedRef<SVerticalBox> Panel = SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f)
		[
			SNew(STextBlock)
				.Text(Title)
		];

	if (Graph)
	{
		// The revision asset arrives without an ed-graph; build it so it can be displayed. The asset is
		// a transient copy (current asset or a temp package loaded from source control), so mutating it is safe.
		UScriptableGraph* MutableGraph = const_cast<UScriptableGraph*>(Graph);
		FScriptableGraphEditor::BuildEdGraphForAsset(MutableGraph);

		if (MutableGraph->EdGraph)
		{
			OutEditor = SNew(SGraphEditor)
				.GraphToEdit(MutableGraph->EdGraph)
				.IsEditable(false);

			Panel->AddSlot()
				.FillHeight(1.0f)
				[
					OutEditor.ToSharedRef()
				];
		}
	}
	else
	{
		Panel->AddSlot()
			.FillHeight(1.0f)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
					.Text(LOCTEXT("NoGraphInRevision", "Asset not present in this revision"))
			];
	}

	return Panel;
}

#undef LOCTEXT_NAMESPACE

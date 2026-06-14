// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/Diff/SScriptableGraphDiff.h"
#include "ScriptableFrameworkEd/Graph/Diff/ScriptableGraphToDiff.h"
#include "ScriptableFrameworkEd/Graph/Editor/ScriptableGraphEditor.h"
#include "ScriptableNodes/ScriptableGraph.h"

#include "DiffUtils.h"
#include "DetailsDiff.h"
#include "GraphEditor.h"
#include "EdGraph/EdGraph.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SWindow.h"
#include "Styling/AppStyle.h"

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

void SScriptableGraphDiff::BuildAssetPropertyDifferences(const UScriptableGraph* OldGraph, const UScriptableGraph* NewGraph)
{
	TArray<TSharedPtr<FBlueprintDifferenceTreeEntry>> Children;

	// Both revisions present: diff the asset's editable fields. The details view only surfaces
	// EditAnywhere/VisibleAnywhere properties (Context, Locals, Outputs), so Nodes/Connections —
	// already covered by the graph diff — don't show up here.
	if (OldGraph && NewGraph)
	{
		FDetailsDiff OldDetails(OldGraph);
		FDetailsDiff NewDetails(NewGraph);

		TArray<FSingleObjectDiffEntry> PropertyDiffs;
		OldDetails.DiffAgainst(NewDetails, PropertyDiffs);

		for (const FSingleObjectDiffEntry& PropertyDiff : PropertyDiffs)
		{
			const FText Label = FText::FromString(PropertyDiff.Identifier.ToDisplayName());
			TSharedPtr<FBlueprintDifferenceTreeEntry> Entry = MakeShared<FBlueprintDifferenceTreeEntry>(
				FOnDiffEntryFocused(),
				FGenerateDiffEntryWidget::CreateLambda([Label]()
				{
					return StaticCastSharedRef<SWidget>(SNew(STextBlock)
						.Text(Label)
						.ColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.8f, 0.3f))));
				}));

			Children.Add(Entry);
			RealDifferences.Add(Entry);
		}
	}

	const bool bHasDifferences = Children.Num() > 0;
	if (Children.Num() == 0)
	{
		Children.Add(FBlueprintDifferenceTreeEntry::NoDifferencesEntry());
	}

	PrimaryDifferencesList.Add(FBlueprintDifferenceTreeEntry::CreateCategoryEntry(
		LOCTEXT("AssetCategory", "Asset Properties"),
		LOCTEXT("AssetCategoryTooltip", "Changes to the graph's Context, Locals and Outputs."),
		FOnDiffEntryFocused(),
		Children,
		bHasDifferences));
}

void SScriptableGraphDiff::Construct(const FArguments& InArgs)
{
	// Build the graph editors first; their ed-graphs are what the diff control compares.
	const TSharedRef<SWidget> OldPanel = BuildGraphEditor(InArgs._OldGraph, MakeRevisionLabel(InArgs._OldRevision, LOCTEXT("OldRevision", "Old Revision")), OldGraphEditor);
	const TSharedRef<SWidget> NewPanel = BuildGraphEditor(InArgs._NewGraph, MakeRevisionLabel(InArgs._NewRevision, LOCTEXT("NewRevision", "New / Local")), NewGraphEditor);

	UEdGraph* OldEdGraph = InArgs._OldGraph ? InArgs._OldGraph->EdGraph : nullptr;
	UEdGraph* NewEdGraph = InArgs._NewGraph ? InArgs._NewGraph->EdGraph : nullptr;

	GraphToDiff = MakeShared<FScriptableGraphToDiff>(OldEdGraph, NewEdGraph,
		FScriptableGraphToDiff::FOnFocusDiff::CreateSP(this, &SScriptableGraphDiff::OnFocusDiff));
	GraphToDiff->GenerateTreeEntries(PrimaryDifferencesList, RealDifferences);

	BuildAssetPropertyDifferences(InArgs._OldGraph, InArgs._NewGraph);

	DifferencesTreeView = DiffTreeView::CreateTreeView(&PrimaryDifferencesList);
	for (const TSharedPtr<FBlueprintDifferenceTreeEntry>& Root : PrimaryDifferencesList)
	{
		DifferencesTreeView->SetItemExpansion(Root, true);
	}

	ChildSlot
	[
		SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.0f)
			[
				SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
							.Text(LOCTEXT("PrevDiff", "Prev"))
							.IsEnabled(this, &SScriptableGraphDiff::HasPrevDiff)
							.OnClicked_Lambda([this]() { PrevDiff(); return FReply::Handled(); })
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(4.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(SButton)
							.Text(LOCTEXT("NextDiff", "Next"))
							.IsEnabled(this, &SScriptableGraphDiff::HasNextDiff)
							.OnClicked_Lambda([this]() { NextDiff(); return FReply::Handled(); })
					]
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SSplitter)
					+ SSplitter::Slot()
					.Value(0.2f)
					[
						SNew(SBorder)
							.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
							[
								DifferencesTreeView.ToSharedRef()
							]
					]
					+ SSplitter::Slot()
					.Value(0.8f)
					[
						SNew(SSplitter)
							+ SSplitter::Slot().Value(0.5f)[ OldPanel ]
							+ SSplitter::Slot().Value(0.5f)[ NewPanel ]
					]
			]
	];
}

TSharedRef<SWidget> SScriptableGraphDiff::BuildGraphEditor(const UScriptableGraph* Graph, const FText& Title, TSharedPtr<SGraphEditor>& OutEditor)
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
		// The revision asset arrives without an ed-graph; build it so it can be displayed and diffed.
		// The asset is a transient copy (current asset or a temp package from source control), so mutating it is safe.
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

void SScriptableGraphDiff::OnFocusDiff(const FDiffSingleResult& Diff)
{
	if (Diff.Node1 && OldGraphEditor.IsValid())
	{
		OldGraphEditor->JumpToNode(Diff.Node1, /*bRequestRename*/ false, /*bSelectNode*/ true);
	}
	if (Diff.Node2 && NewGraphEditor.IsValid())
	{
		NewGraphEditor->JumpToNode(Diff.Node2, /*bRequestRename*/ false, /*bSelectNode*/ true);
	}
}

void SScriptableGraphDiff::NextDiff()
{
	if (DifferencesTreeView.IsValid())
	{
		DiffTreeView::HighlightNextDifference(DifferencesTreeView.ToSharedRef(), RealDifferences, PrimaryDifferencesList);
	}
}

void SScriptableGraphDiff::PrevDiff()
{
	if (DifferencesTreeView.IsValid())
	{
		DiffTreeView::HighlightPrevDifference(DifferencesTreeView.ToSharedRef(), RealDifferences, PrimaryDifferencesList);
	}
}

bool SScriptableGraphDiff::HasNextDiff() const
{
	return DifferencesTreeView.IsValid() && DiffTreeView::HasNextDifference(DifferencesTreeView.ToSharedRef(), RealDifferences);
}

bool SScriptableGraphDiff::HasPrevDiff() const
{
	return DifferencesTreeView.IsValid() && DiffTreeView::HasPrevDifference(DifferencesTreeView.ToSharedRef(), RealDifferences);
}

#undef LOCTEXT_NAMESPACE

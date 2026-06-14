// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/Diff/ScriptableGraphToDiff.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode.h"
#include "ScriptableNodes/ScriptableNode.h"

#include "DiffUtils.h"
#include "DetailsDiff.h"
#include "GraphDiffControl.h"
#include "EdGraph/EdGraph.h"
#include "Algo/Sort.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ScriptableGraphToDiff"

namespace
{
	FLinearColor ColorForCategory(EDiffType::Category Category)
	{
		switch (Category)
		{
		case EDiffType::Category::ADDITION:     return FLinearColor(0.3f, 1.0f, 0.3f);
		case EDiffType::Category::SUBTRACTION:  return FLinearColor(1.0f, 0.3f, 0.3f);
		case EDiffType::Category::MODIFICATION: return FLinearColor(1.0f, 0.8f, 0.3f);
		default:                                return FLinearColor(0.6f, 0.6f, 0.6f);
		}
	}
}

FScriptableGraphToDiff::FScriptableGraphToDiff(UEdGraph* InOldGraph, UEdGraph* InNewGraph, FOnFocusDiff InOnFocusDiff)
	: OldGraph(InOldGraph)
	, NewGraph(InNewGraph)
	, OnFocusDiff(InOnFocusDiff)
{
	// Structural differences (added/removed nodes, pin relinks, moves).
	FGraphDiffControl::DiffGraphs(OldGraph, NewGraph, FoundDiffs);

	// DiffGraphs doesn't look inside a node's instanced task; add those property changes ourselves.
	AppendNodePropertyDiffs();

	// Group like differences (added, removed, moved, property...) so the tree reads consistently.
	Algo::SortBy(FoundDiffs, &FDiffSingleResult::Diff);
}

void FScriptableGraphToDiff::AppendNodePropertyDiffs()
{
	if (!OldGraph || !NewGraph)
	{
		return;
	}

	// Match nodes across revisions by their stable BindingID (survives reorders and reconstruction).
	TMap<FGuid, UScriptableEdGraphNode*> OldNodesByID;
	for (UEdGraphNode* Node : OldGraph->Nodes)
	{
		if (UScriptableEdGraphNode* EdNode = Cast<UScriptableEdGraphNode>(Node))
		{
			if (const UScriptableNode* RuntimeNode = EdNode->GetRuntimeNode())
			{
				OldNodesByID.Add(RuntimeNode->GetBindingID(), EdNode);
			}
		}
	}

	for (UEdGraphNode* Node : NewGraph->Nodes)
	{
		UScriptableEdGraphNode* NewEdNode = Cast<UScriptableEdGraphNode>(Node);
		UScriptableNode* NewRuntimeNode = NewEdNode ? NewEdNode->GetRuntimeNode() : nullptr;
		if (!NewRuntimeNode)
		{
			continue;
		}

		UScriptableEdGraphNode* OldEdNode = OldNodesByID.FindRef(NewRuntimeNode->GetBindingID());
		UScriptableNode* OldRuntimeNode = OldEdNode ? OldEdNode->GetRuntimeNode() : nullptr;
		if (!OldRuntimeNode)
		{
			// Added node: structural diff already reports it.
			continue;
		}

		// FDetailsDiff expands instanced subobjects (the node's Task), so its property diff covers
		// task fields too — something a raw reflection compare misses. The detail views are transient,
		// only needed to extract the entries here.
		FDetailsDiff OldDetails(OldRuntimeNode);
		FDetailsDiff NewDetails(NewRuntimeNode);

		TArray<FSingleObjectDiffEntry> PropertyDiffs;
		OldDetails.DiffAgainst(NewDetails, PropertyDiffs);

		const FString NodeTitle = NewEdNode->GetNodeTitle(ENodeTitleType::ListView).ToString();
		for (const FSingleObjectDiffEntry& PropertyDiff : PropertyDiffs)
		{
			FDiffSingleResult Synthetic;
			Synthetic.Diff = EDiffType::NODE_PROPERTY;
			Synthetic.Category = EDiffType::Category::MODIFICATION;
			Synthetic.Node1 = OldEdNode;
			Synthetic.Node2 = NewEdNode;
			Synthetic.DisplayString = FText::Format(
				LOCTEXT("PropertyDiffFmt", "{0}: {1}"),
				FText::FromString(NodeTitle),
				FText::FromString(PropertyDiff.Identifier.ToDisplayName()));
			FoundDiffs.Add(Synthetic);
		}
	}
}

void FScriptableGraphToDiff::GenerateTreeEntries(TArray<TSharedPtr<FBlueprintDifferenceTreeEntry>>& OutTreeEntries, TArray<TSharedPtr<FBlueprintDifferenceTreeEntry>>& OutRealDifferences)
{
	TArray<TSharedPtr<FBlueprintDifferenceTreeEntry>> Children;

	for (const FDiffSingleResult& Diff : FoundDiffs)
	{
		if (!Diff.IsRealDifference())
		{
			continue;
		}

		TSharedPtr<FBlueprintDifferenceTreeEntry> Entry = MakeShared<FBlueprintDifferenceTreeEntry>(
			FOnDiffEntryFocused::CreateSP(this, &FScriptableGraphToDiff::HandleFocusDiff, Diff),
			FGenerateDiffEntryWidget::CreateSP(this, &FScriptableGraphToDiff::CreateDiffRowWidget, Diff));

		Children.Add(Entry);
		OutRealDifferences.Add(Entry);
	}

	if (Children.Num() == 0)
	{
		Children.Add(FBlueprintDifferenceTreeEntry::NoDifferencesEntry());
	}

	OutTreeEntries.Add(FBlueprintDifferenceTreeEntry::CreateCategoryEntry(
		LOCTEXT("GraphCategory", "Graph"),
		LOCTEXT("GraphCategoryTooltip", "Differences in the graph's nodes and connections."),
		FOnDiffEntryFocused(),
		Children,
		/*bHasDifferences*/ Children.Num() > 0 && FoundDiffs.Num() > 0));
}

TSharedRef<SWidget> FScriptableGraphToDiff::CreateDiffRowWidget(FDiffSingleResult Diff) const
{
	return SNew(STextBlock)
		.Text(Diff.DisplayString)
		.ToolTipText(Diff.ToolTip)
		.ColorAndOpacity(FSlateColor(ColorForCategory(Diff.Category)));
}

void FScriptableGraphToDiff::HandleFocusDiff(FDiffSingleResult Diff) const
{
	OnFocusDiff.ExecuteIfBound(Diff);
}

#undef LOCTEXT_NAMESPACE

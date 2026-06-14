// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "DiffResults.h"

class FBlueprintDifferenceTreeEntry;
class UEdGraph;

/**
 * Diffs two Scriptable Graph ed-graphs with the engine's FGraphDiffControl and turns the results
 * into difference-tree entries (added/removed nodes, pin relinks, moves...). Owns nothing visual;
 * the host widget renders the tree and reacts to focus by jumping both graph panels to the node.
 */
class FScriptableGraphToDiff : public TSharedFromThis<FScriptableGraphToDiff>
{
public:
	DECLARE_DELEGATE_OneParam(FOnFocusDiff, const FDiffSingleResult&);

	FScriptableGraphToDiff(UEdGraph* InOldGraph, UEdGraph* InNewGraph, FOnFocusDiff InOnFocusDiff);

	/** Appends a "Graph" category whose children are one entry per real difference. */
	void GenerateTreeEntries(TArray<TSharedPtr<FBlueprintDifferenceTreeEntry>>& OutTreeEntries, TArray<TSharedPtr<FBlueprintDifferenceTreeEntry>>& OutRealDifferences);

private:
	/** Pairs nodes across revisions by BindingID and appends a synthetic property-change diff per altered task property. */
	void AppendNodePropertyDiffs();

	TSharedRef<SWidget> CreateDiffRowWidget(FDiffSingleResult Diff) const;
	void HandleFocusDiff(FDiffSingleResult Diff) const;

	UEdGraph* OldGraph = nullptr;
	UEdGraph* NewGraph = nullptr;
	FOnFocusDiff OnFocusDiff;

	TArray<FDiffSingleResult> FoundDiffs;
};

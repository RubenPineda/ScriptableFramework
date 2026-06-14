// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"
#include "AssetDefinition.h"
#include "DiffResults.h"

class UScriptableGraph;
class SGraphEditor;
class FScriptableGraphToDiff;
class FBlueprintDifferenceTreeEntry;

/**
 * Visual diff between two Scriptable Graph revisions in a standalone window: a difference tree on
 * the left (with prev/next navigation), the old and new graphs side by side on the right. Selecting
 * a difference jumps both graph panels to the affected node. Either revision may be null when the
 * asset didn't exist there.
 */
class SScriptableGraphDiff : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SScriptableGraphDiff) {}
		SLATE_ARGUMENT(const UScriptableGraph*, OldGraph)
		SLATE_ARGUMENT(const UScriptableGraph*, NewGraph)
		SLATE_ARGUMENT(FRevisionInfo, OldRevision)
		SLATE_ARGUMENT(FRevisionInfo, NewRevision)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Opens a window diffing the two revisions. Either graph may be null. */
	static void CreateDiffWindow(const UScriptableGraph* OldGraph, const UScriptableGraph* NewGraph, const FRevisionInfo& OldRevision, const FRevisionInfo& NewRevision);

private:
	/** Ensures Graph has an ed-graph and builds a labelled read-only graph editor for it (placeholder when null). */
	TSharedRef<SWidget> BuildGraphEditor(const UScriptableGraph* Graph, const FText& Title, TSharedPtr<SGraphEditor>& OutEditor);

	static FText MakeRevisionLabel(const FRevisionInfo& Revision, const FText& Fallback);

	/** Centers both graph panels on the node(s) the focused difference refers to. */
	void OnFocusDiff(const FDiffSingleResult& Diff);

	void NextDiff();
	void PrevDiff();
	bool HasNextDiff() const;
	bool HasPrevDiff() const;

	TSharedPtr<SGraphEditor> OldGraphEditor;
	TSharedPtr<SGraphEditor> NewGraphEditor;

	TSharedPtr<FScriptableGraphToDiff> GraphToDiff;

	/** Root entries shown in the tree (categories); RealDifferences is the flat list used for navigation. */
	TArray<TSharedPtr<FBlueprintDifferenceTreeEntry>> PrimaryDifferencesList;
	TArray<TSharedPtr<FBlueprintDifferenceTreeEntry>> RealDifferences;
	TSharedPtr<STreeView<TSharedPtr<FBlueprintDifferenceTreeEntry>>> DifferencesTreeView;
};

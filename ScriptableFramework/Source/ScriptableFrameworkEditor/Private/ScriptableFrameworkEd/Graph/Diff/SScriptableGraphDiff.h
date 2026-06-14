// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "AssetDefinition.h"

class UScriptableGraph;
class SGraphEditor;

/**
 * Visual diff between two Scriptable Graph revisions, shown in a standalone window. Either side may
 * be null when the asset didn't exist in that revision. Currently shows both graphs side by side;
 * the difference tree and property diff are layered on next.
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
	/** Builds a labelled panel hosting a read-only graph editor for Graph (or a placeholder when null). */
	TSharedRef<SWidget> BuildGraphPanel(const UScriptableGraph* Graph, const FText& Title, TSharedPtr<SGraphEditor>& OutEditor);

	/** Revision label, falling back to a generic text when no revision string is available. */
	static FText MakeRevisionLabel(const FRevisionInfo& Revision, const FText& Fallback);

	TSharedPtr<SGraphEditor> OldGraphEditor;
	TSharedPtr<SGraphEditor> NewGraphEditor;
};

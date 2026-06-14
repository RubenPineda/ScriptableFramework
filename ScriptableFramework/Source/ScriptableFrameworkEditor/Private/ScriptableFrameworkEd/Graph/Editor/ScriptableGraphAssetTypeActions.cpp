// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/Editor/ScriptableGraphAssetTypeActions.h"
#include "ScriptableFrameworkEd/Graph/Diff/SScriptableGraphDiff.h"
#include "ScriptableNodes/ScriptableGraph.h"

void FScriptableGraphAssetTypeActions::PerformAssetDiff(UObject* OldAsset, UObject* NewAsset, const FRevisionInfo& OldRevision, const FRevisionInfo& NewRevision) const
{
	const UScriptableGraph* OldGraph = Cast<UScriptableGraph>(OldAsset);
	const UScriptableGraph* NewGraph = Cast<UScriptableGraph>(NewAsset);
	if (!OldGraph && !NewGraph)
	{
		return;
	}

	SScriptableGraphDiff::CreateDiffWindow(OldGraph, NewGraph, OldRevision, NewRevision);
}

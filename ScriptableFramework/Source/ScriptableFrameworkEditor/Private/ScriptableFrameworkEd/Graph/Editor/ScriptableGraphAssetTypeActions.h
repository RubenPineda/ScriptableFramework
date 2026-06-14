// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "AssetTools/KzAssetTypeActions_Base.h"
#include "ScriptableFrameworkEd/Graph/Editor/ScriptableGraphEditor.h"

/**
 * Asset actions for UScriptableGraph. Reuses the KzLib template for opening the graph editor and
 * adds PerformAssetDiff so the source-control "Diff against revision" menu opens the graph diff tool.
 */
class FScriptableGraphAssetTypeActions : public TKzAssetTypeActions<FScriptableGraphEditor>
{
public:
	using TKzAssetTypeActions<FScriptableGraphEditor>::TKzAssetTypeActions;

	virtual void PerformAssetDiff(UObject* OldAsset, UObject* NewAsset, const struct FRevisionInfo& OldRevision, const struct FRevisionInfo& NewRevision) const override;
};

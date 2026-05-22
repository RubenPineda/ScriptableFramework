// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "GraphEditor.h"

class UScriptableGraph;
class UEdGraph;
class UEdGraphPin;
class IDetailsView;
class SGraphEditor;

/** Asset editor for UScriptableGraph. */
class FScriptableGraphEditor : public FAssetEditorToolkit
{
public:
	/** Entry point matched by TKzAssetTypeActions: opens the toolkit on the supplied assets. */
	static void CreateEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, const TArray<UObject*>& InObjects);

	/** Initializes the editor and opens the toolkit window for the given graph asset. */
	void Initialize(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UScriptableGraph* InGraph);

	//~ IToolkit interface
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	//~ End of IToolkit interface

private:
	TSharedRef<SDockTab> SpawnTab_Graph(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_AssetDetails(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_NodeDetails(const FSpawnTabArgs& Args);

	/** Creates the asset's UEdGraph the first time it is opened (after the runtime-only era). */
	void InitEdGraph();

	/** Reconstructs visual UEdGraphNodes from the asset's runtime Nodes list, skipping any already represented. */
	void ReconstructEdGraphFromAsset();

	/** Builds the context menu shown when the user right-clicks on empty graph space or drags from a pin. */
	FActionMenuContent OnCreateNodeMenu(UEdGraph* InGraph, const FVector2f& InNodePosition, const TArray<UEdGraphPin*>& InDraggedPins, bool bAutoExpand, SGraphEditor::FActionMenuClosed InOnMenuClosed);

	/** Callback invoked by the SScriptableTypeSelector when the user picks a class or asset from the popup. */
	void OnNodeMenuTypePicked(const UStruct* InStruct, const FAssetData& InAssetData, UEdGraph* InGraph, FVector2f InLocation, TArray<UEdGraphPin*> InDraggedPins);

	/** The asset currently being edited. */
	TWeakObjectPtr<UScriptableGraph> EditedGraph;

	/** Details view bound to the asset. */
	TSharedPtr<IDetailsView> AssetDetailsView;

	/** Details view bound to the currently selected node (empty until selection is implemented). */
	TSharedPtr<IDetailsView> NodeDetailsView;

	/** The graph editor widget shown in the center tab. */
	TSharedPtr<SGraphEditor> GraphEditorWidget;

	static const FName GraphTabId;
	static const FName AssetDetailsTabId;
	static const FName NodeDetailsTabId;
};
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

	virtual ~FScriptableGraphEditor();

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
	TSharedRef<SDockTab> SpawnTab_Palette(const FSpawnTabArgs& Args);

	/** Creates the asset's UEdGraph the first time it is opened (after the runtime-only era). */
	void InitEdGraph();

	/** Reconstructs visual UEdGraphNodes from the asset's runtime Nodes list, skipping any already represented. */
	void ReconstructEdGraphFromAsset();

	/** Builds the context menu shown when the user right-clicks on empty graph space or drags from a pin. */
	FActionMenuContent OnCreateNodeMenu(UEdGraph* InGraph, const FVector2f& InNodePosition, const TArray<UEdGraphPin*>& InDraggedPins, bool bAutoExpand, SGraphEditor::FActionMenuClosed InOnMenuClosed);

	/** Callback invoked by the SScriptableTypeSelector when the user picks a class or asset from the popup. */
	void OnNodeMenuTypePicked(const UStruct* InStruct, const FAssetData& InAssetData, UEdGraph* InGraph, FVector2f InLocation, TArray<UEdGraphPin*> InDraggedPins);

	/** Palette variant of OnNodeMenuTypePicked: same dispatch, but synthesizes a default spawn position (canvas center) since the palette isn't a drag-off context. */
	void OnPaletteTypePicked(const UStruct* InStruct, const FAssetData& InAssetData);

	/** Routes the graph selection into the node details panel; Task nodes are unwrapped so the inner task's properties show. Empty or multi-selection clears the panel. */
	void OnGraphSelectionChanged(const FGraphPanelSelectionSet& NewSelection);

	/** Reconstructs the ed-node wrapping the runtime node that owns the changed property, so dynamic node titles (and any future pin-changing customizations) stay in sync with the details panel edits. */
	void OnRuntimeNodePropertyChanged(UObject* InObject, FPropertyChangedEvent& InEvent);

	/** Pin-context command executor: removes the right-clicked Sequence output pin (or no-op if the selected pin isn't a removable Sequence branch). Reads the target pin from the GraphEditorWidget's current selection set, since FUICommandInfo executors don't receive the pin directly. */
	void OnRemoveSequencePin();

	/** Enables the Remove pin entry only when the right-clicked pin belongs to a UScriptableNode_Sequence with OutputCount > 1 and is an output. */
	bool CanRemoveSequencePin() const;

	/** Pin-context executor: removes the right-clicked AND input pin. Reads the target pin from the GraphEditorWidget's current selection. */
	void OnRemoveANDPin();

	/** Greys out the AND Remove-pin entry once the node would dip below MinInputCount. */
	bool CanRemoveANDPin() const;

	/** Hooked to FCoreUObjectDelegates::OnObjectTransacted. Fires after UE applies an undo or redo to any object; we filter to the edited asset and rebuild the ed-graph to match the restored Connections / Nodes state. This is how pin-removal undo works without us doing in-place pin manipulation inside the transaction. */
	void OnObjectTransacted(UObject* Object, const FTransactionObjectEvent& Event);

	/** Maps generic Slate commands (delete, copy, cut, paste, duplicate, select all, undo, redo) into the graph editor. */
	void BindGraphCommands();

	// --- Selection helpers ---
	bool HasAnyNodesSelected() const;
	bool HasAnyDeletableSelected() const;
	bool HasAnyCopyableSelected() const;

	// --- Command handlers ---
	void OnDeleteSelected();
	void OnCopySelected();
	void OnCutSelected();
	void OnPasteAtCursor();
	void OnDuplicateSelected();
	void OnSelectAllNodes();
	void OnCreateComment();

	bool CanDelete() const { return HasAnyDeletableSelected(); }
	bool CanCopy() const { return HasAnyCopyableSelected(); }
	bool CanCut() const { return HasAnyDeletableSelected() && HasAnyCopyableSelected(); }
	bool CanPaste() const;
	bool CanDuplicate() const { return HasAnyCopyableSelected(); }
	bool CanSelectAll() const { return GraphEditorWidget.IsValid(); }

	/** The asset currently being edited. */
	TWeakObjectPtr<UScriptableGraph> EditedGraph;

	/** Details view bound to the asset. */
	TSharedPtr<IDetailsView> AssetDetailsView;

	/** Details view bound to the currently selected node (empty until selection is implemented). */
	TSharedPtr<IDetailsView> NodeDetailsView;

	/** The graph editor widget shown in the center tab. */
	TSharedPtr<SGraphEditor> GraphEditorWidget;

	/** Handle to the editor-wide property change broadcast, kept so we can unsubscribe at destruction. */
	FDelegateHandle OnObjectPropertyChangedHandle;
	FDelegateHandle OnObjectTransactedHandle;

	static const FName GraphTabId;
	static const FName AssetDetailsTabId;
	static const FName NodeDetailsTabId;
	static const FName PaletteTabId;
};
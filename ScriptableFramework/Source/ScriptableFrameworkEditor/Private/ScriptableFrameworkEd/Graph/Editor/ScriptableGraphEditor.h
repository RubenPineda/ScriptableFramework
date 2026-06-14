// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "GraphEditor.h"
#include "Widgets/Views/SListView.h"
#include "Textures/SlateIcon.h"

struct FEdGraphEditAction;
class FObjectPostSaveContext;

class UScriptableGraph;
class UScriptableNode;
class UEdGraph;
class UEdGraphPin;
class IDetailsView;
class SGraphEditor;
class SSearchBox;
class SKzValidationPanel;
class FScriptableGraphSpawnInputProcessor;
struct FKzValidationIssue;

/** Asset editor for UScriptableGraph. */
class FScriptableGraphEditor : public FAssetEditorToolkit
{
	friend class FScriptableGraphSpawnInputProcessor;

public:
	/** Entry point matched by TKzAssetTypeActions: opens the toolkit on the supplied assets. */
	static void CreateEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, const TArray<UObject*>& InObjects);

	virtual ~FScriptableGraphEditor();

	/** Initializes the editor and opens the toolkit window for the given graph asset. */
	void Initialize(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UScriptableGraph* InGraph);

	/** Pans and selects the given ed-node. External code (e.g. the breakpoint handler) uses this to focus a specific node. */
	void JumpToNode(class UEdGraphNode* Node);

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

public:
	/**
	 * Ensures Graph has a UEdGraph and rebuilds its visual nodes/links from the runtime Nodes and
	 * Connections. Standalone (no toolkit) so the diff tool can build the ed-graph for a revision
	 * loaded outside an editor. Idempotent; does not dirty the package.
	 */
	static void BuildEdGraphForAsset(class UScriptableGraph* Graph);

private:

	/** Builds the context menu shown when the user right-clicks on empty graph space or drags from a pin. */
	FActionMenuContent OnCreateNodeMenu(UEdGraph* InGraph, const FVector2f& InNodePosition, const TArray<UEdGraphPin*>& InDraggedPins, bool bAutoExpand, SGraphEditor::FActionMenuClosed InOnMenuClosed);

	/** Callback invoked by the SScriptableTypeSelector when the user picks a class or asset from the popup. */
	void OnNodeMenuTypePicked(const UStruct* InStruct, const FAssetData& InAssetData, UEdGraph* InGraph, FVector2f InLocation, TArray<UEdGraphPin*> InDraggedPins);

	/** Palette variant of OnNodeMenuTypePicked: spawns at canvas center (no drag-off context). */
	void OnPaletteTypePicked(const UStruct* InStruct, const FAssetData& InAssetData);

	/** Routes selection into the node details panel (Task nodes unwrapped to show the inner task). Empty/multi-selection clears it. */
	void OnGraphSelectionChanged(const FGraphPanelSelectionSet& NewSelection);

	/** Persists an editable node title (e.g. a comment box) via OnRenameNode. SGraphNode routes commits here and provides no built-in persistence. */
	void OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, class UEdGraphNode* NodeBeingChanged);

	/** Opens the asset a node points to (a Blueprint task, an Action asset, or a sub-graph). No-op for nodes with no associated asset. */
	void OnNodeDoubleClicked(class UEdGraphNode* Node);

	/** Rename command (F2): requests inline title editing on the selected renameable node (e.g. an Event node). */
	void OnRenameSelectedNode();
	bool CanRenameSelectedNode() const;

	/** Reconstructs the ed-node whose runtime node changed, keeping dynamic titles/pins in sync with details-panel edits. */
	void OnRuntimeNodePropertyChanged(UObject* InObject, FPropertyChangedEvent& InEvent);

	/** Saving a foreign UScriptableGraph rebuilds every embedded SubGraph node that references it; complements the live property-change path for edits OnObjectPropertyChanged misses (e.g. ReceiveEvent add/remove). */
	void OnPackageSaved(const FString& PackageFileName, UPackage* SavedPackage, FObjectPostSaveContext SaveContext);

	/** Refactor the current selection (minus Entry) into a new UScriptableGraph asset + a SubGraph node that replaces them in the source graph. */
	void OnConvertSelectionToSubGraph();
	bool CanConvertSelectionToSubGraph() const;

	/** Removes the right-clicked Sequence output pin (no-op if not a removable branch). Reads the pin from the widget's selection, since command executors don't receive it. */
	void OnRemoveSequencePin();

	/** Enables Remove pin only for a Sequence output pin when OutputCount > 1. */
	bool CanRemoveSequencePin() const;

	/** Pin-context executor: removes the right-clicked AND input pin. Reads the target pin from the GraphEditorWidget's current selection. */
	void OnRemoveANDPin();

	/** Greys out the AND Remove-pin entry once the node would dip below MinInputCount. */
	bool CanRemoveANDPin() const;

	/** Pin-context executor: removes the right-clicked OR input pin. Mirrors OnRemoveANDPin. */
	void OnRemoveORPin();

	/** Greys out the OR Remove-pin entry once the node would dip below MinInputCount. */
	bool CanRemoveORPin() const;

	/** Fires after any undo/redo (FCoreUObjectDelegates::OnObjectTransacted); filters to the edited asset and rebuilds the ed-graph to match the restored Connections/Nodes. This is how pin-removal undo works. */
	void OnObjectTransacted(UObject* Object, const FTransactionObjectEvent& Event);

	/** Maps generic Slate commands (delete, copy, cut, paste, duplicate, select all, undo, redo) into the graph editor. */
	void BindGraphCommands();

	// --- Selection helpers ---
	bool HasAnyNodesSelected() const;
	bool HasAnyDeletableSelected() const;
	bool HasAnyCopyableSelected() const;

	// --- Command handlers ---
	void OnDeleteSelected();
	void OnDeleteAndReconnectNodes();
	void OnCopySelected();
	void OnCutSelected();
	void OnPasteAtCursor();
	void OnDuplicateSelected();
	void OnSelectAllNodes();
	void OnCreateComment();

	/** Cardinal alignment (Mode picks min/max/center) along the chosen axis. */
	enum class EScriptableAlignMode : uint8 { Min, Max, Center };
	void AlignNodesAxis(EScriptableAlignMode Mode, bool bVertical);
	void OnAlignNodesTop()    { AlignNodesAxis(EScriptableAlignMode::Min,    /*bVertical*/ true);  }
	void OnAlignNodesBottom() { AlignNodesAxis(EScriptableAlignMode::Max,    /*bVertical*/ true);  }
	void OnAlignNodesMiddle() { AlignNodesAxis(EScriptableAlignMode::Center, /*bVertical*/ true);  }
	void OnAlignNodesLeft()   { AlignNodesAxis(EScriptableAlignMode::Min,    /*bVertical*/ false); }
	void OnAlignNodesRight()  { AlignNodesAxis(EScriptableAlignMode::Max,    /*bVertical*/ false); }
	void OnAlignNodesCenter() { AlignNodesAxis(EScriptableAlignMode::Center, /*bVertical*/ false); }

	/** Even spacing of the selection between the first and last node on the chosen axis. */
	void DistributeNodesAxis(bool bVertical);
	void OnDistributeNodesHorizontally() { DistributeNodesAxis(/*bVertical*/ false); }
	void OnDistributeNodesVertically()   { DistributeNodesAxis(/*bVertical*/ true);  }

	/** Pan and zoom the graph view to fit the current selection. No-op when nothing is selected. */
	void OnZoomToSelection();

	/** BP-style F9: each selected scriptable node toggles between "no breakpoint" and "enabled breakpoint". */
	void OnToggleBreakpoint();

	/** Spawns the given runtime UScriptableNode subclass at the current cursor position in graph space.
	 * Called by FScriptableGraphSpawnInputProcessor when the user clicks while holding a shortcut key. */
	void OnSpawnNativeNodeAtCursor(UClass* RuntimeNodeClass);

	/** Returns the current mouse cursor in graph-space coordinates, falling back to the cached paste
	 * location when the panel geometry is not yet resolved. */
	FVector2f GetCursorGraphPosition() const;

	/** Exposed to the input processor so it can test whether the click landed inside our graph editor. */
	TSharedPtr<SGraphEditor> GetGraphEditorWidget() const { return GraphEditorWidget; }

	bool CanDelete() const { return HasAnyDeletableSelected(); }
	bool CanDeleteAndReconnectNodes() const;
	bool CanCopy() const { return HasAnyCopyableSelected(); }
	bool CanCut() const { return HasAnyDeletableSelected() && HasAnyCopyableSelected(); }
	bool CanPaste() const;
	bool CanDuplicate() const { return HasAnyCopyableSelected(); }
	bool CanSelectAll() const { return GraphEditorWidget.IsValid(); }

	// --- Validation ---
	/** Spawns the Validation tab hosting an SKzValidationPanel wired to this graph. */
	TSharedRef<SDockTab> SpawnTab_Validation(const FSpawnTabArgs& Args);

	/** Adds the "Compile" toolbar button. */
	void ExtendToolbar();

	/** Toolbar handler: runs the validator pass, persists the compile result on the asset, and surfaces issues in the panel. */
	void OnCompile();

	/** Shared compile flow used by both the toolbar button and the open-time pass. Set bMarkPackageDirty=false when the flip is a passive refresh, not a user edit. */
	void RunCompile(bool bMarkPackageDirty);

	/** Marks the editor dirty since the last compile. Called from every semantic edit hook. */
	void MarkDirtySinceLastCompile();

	/** UEdGraph::OnGraphChanged hook: marks dirty on any change that isn't a pure selection. */
	void HandleGraphChanged(const FEdGraphEditAction& Action);

	/** Applies validation issues onto each ed-node's bHasCompilerMessage/ErrorType/ErrorMsg, then triggers a redraw. */
	void ApplyValidationToErrorBanners(const TArray<FKzValidationIssue>& Issues);

	/** Re-runs validation and refreshes per-node ERROR!/WARNING! banners; cheap variant of HandleRunValidation without details-panel rebuilds, suitable for live edit hooks. */
	void RefreshErrorBanners();

	/** Driven by bIsDirtySinceLastCompile and Graph->bLastCompileFailed: yellow > red > green. */
	FSlateIcon GetCompileButtonIcon() const;
	FText GetCompileButtonTooltip() const;

	/** Dropdown shown by the small chevron next to Compile: Save on Compile submenu + Jump to Error Node toggle. */
	TSharedRef<SWidget> GenerateCompileOptionsMenu();
	void BuildSaveOnCompileMenu(class FMenuBuilder& MenuBuilder);

	/** BP-style Debug Object combo: lists live runners of the edited asset, lets the user focus the canvas overlay on one. */
	FText GetDebugObjectLabel() const;
	TSharedRef<SWidget> BuildDebugObjectMenu();

	/** Pull from the panel (Refresh button): re-runs the validator and returns the fresh issue list. Does not persist. */
	TArray<FKzValidationIssue> HandleRunValidation();

	/** Click-to-navigate: selects and pans to the ed-node whose runtime BindingID matches the issue's ContextId. */
	void HandleValidationIssueActivated(const FKzValidationIssue& Issue);

	// --- Graph tools ---
	/** Toolbar handler: pans the graph view to the Entry node. */
	void OnGoHome();

	/** Toolbar handler: deletes every node unreachable from Entry or any ReceiveEvent. */
	void OnCleanGraph();

	/** Toolbar handler: opens the Search tab and focuses its search box. */
	void OnOpenSearch();

	/** Spawns the Search tab: a node filter list wired to click-to-navigate. */
	TSharedRef<SDockTab> SpawnTab_Search(const FSpawnTabArgs& Args);

	/** Rebuilds the search results from the current query and refreshes the list. */
	void OnSearchTextChanged(const FText& InText);

	/** Builds a search result row showing the node's label. */
	TSharedRef<ITableRow> OnGenerateSearchRow(TWeakObjectPtr<UScriptableNode> Item, const TSharedRef<STableViewBase>& OwnerTable);

	/** Click-to-navigate from a search result to its node. */
	void OnSearchResultClicked(TWeakObjectPtr<UScriptableNode> Item);

	/** Returns the ed-node wrapping the runtime node with the given BindingID, or null. */
	UEdGraphNode* FindEdNodeByRuntimeId(const FGuid& RuntimeId) const;

	/** The asset currently being edited. */
	TWeakObjectPtr<UScriptableGraph> EditedGraph;

	/** Details view bound to the asset. */
	TSharedPtr<IDetailsView> AssetDetailsView;

	/** Details view bound to the currently selected node (empty until selection is implemented). */
	TSharedPtr<IDetailsView> NodeDetailsView;

	/** The graph editor widget shown in the center tab. */
	TSharedPtr<SGraphEditor> GraphEditorWidget;

	/** Validation panel shown in the Validation tab. */
	TSharedPtr<SKzValidationPanel> ValidationPanel;

	/** Search tab widgets + current results (runtime nodes matching the query). */
	TSharedPtr<SSearchBox> SearchBox;
	TSharedPtr<SListView<TWeakObjectPtr<UScriptableNode>>> SearchListView;
	TArray<TWeakObjectPtr<UScriptableNode>> SearchResults;

	/** Handle to the editor-wide property change broadcast, kept so we can unsubscribe at destruction. */
	FDelegateHandle OnObjectPropertyChangedHandle;

	/** Handle to UPackage::PackageSavedWithContextEvent, kept so we can unsubscribe at destruction. */
	FDelegateHandle OnPackageSavedHandle;

	/** Handle to UEdGraph::OnGraphChanged so we can detect topology/connection edits and unsubscribe at destruction. */
	FDelegateHandle GraphChangedHandle;

	/** Cleared by RunCompile, set by every semantic edit. Drives the yellow/red status of the Compile button. Transient. */
	bool bIsDirtySinceLastCompile = false;

	/** Set by RunCompile while it broadcasts NotifyGraphChanged to redraw error banners. Stops the dirty tracker from rebooting itself. */
	bool bSuppressDirtyOnGraphChange = false;

	/** Tracks held shortcut keys (S/B/G/E/A) and turns LMB-down over the graph into a node spawn. */
	TSharedPtr<FScriptableGraphSpawnInputProcessor> SpawnInputProcessor;
	FDelegateHandle OnObjectTransactedHandle;

	static const FName GraphTabId;
	static const FName AssetDetailsTabId;
	static const FName NodeDetailsTabId;
	static const FName PaletteTabId;
	static const FName ValidationTabId;
	static const FName SearchTabId;
};
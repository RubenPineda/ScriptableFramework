// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/ScriptableGraphEditor.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraph.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphSchema.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode_Entry.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode_Task.h"
#include "ScriptableNodes/ScriptableNode_Task.h"
#include "ScriptableFrameworkEd/Graph/ScriptableGraphEditorHelpers.h"
#include "ScriptableFrameworkEd/Customization/Widgets/SScriptableTypePicker.h"
#include "ScriptableFrameworkEditorHelpers.h"
#include "ScriptableNodes/ScriptableGraph.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableNodes/ScriptableNode_Entry.h"
#include "ScriptableNodes/ScriptableNode_Task.h"
#include "ScriptableTasks/ScriptableTask.h"

#include "GraphEditor.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Utils/KzEditorUtils.h"
#include "Bindings/ScriptablePropertyBindings.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "ScriptableGraphEditor"

namespace
{
	// Marker line prefixes for the clipboard header. Lines are T3D-compatible comments so the
	// engine's text factory skips them when it scans for object exports.
	constexpr const TCHAR* PositionHeaderPrefix = TEXT(";SF_POS:");
	constexpr const TCHAR* ConnectionHeaderPrefix = TEXT(";SF_CONN:");

	struct FCopiedPositionEntry
	{
		FGuid OldNodeID;
		FVector2f Offset; // relative to selection centroid
	};

	struct FCopiedConnectionEntry
	{
		FGuid OldFromNodeID;
		FName FromPinName;
		FGuid OldToNodeID;
		FName ToPinName;
	};

	/** Builds the clipboard header (positions + internal connections) for a selection. */
	FString BuildClipboardHeader(const TArray<UScriptableEdGraphNode*>& SelectedEdNodes, const UScriptableGraph& Asset)
	{
		if (SelectedEdNodes.IsEmpty()) return FString();

		// Centroid: anchor relative to which each node's offset is stored. Pasting later applies
		// (cursor - centroid) to every offset, preserving the original group layout.
		FVector2f Centroid(0.f, 0.f);
		for (const UScriptableEdGraphNode* EdNode : SelectedEdNodes)
		{
			Centroid.X += EdNode->NodePosX;
			Centroid.Y += EdNode->NodePosY;
		}
		Centroid /= static_cast<float>(SelectedEdNodes.Num());

		// Snapshot the runtime IDs in the selection: connections crossing the boundary are dropped.
		TSet<FGuid> SelectedRuntimeIDs;
		SelectedRuntimeIDs.Reserve(SelectedEdNodes.Num());
		for (const UScriptableEdGraphNode* EdNode : SelectedEdNodes)
		{
			if (const UScriptableNode* Runtime = EdNode->GetRuntimeNode())
			{
				SelectedRuntimeIDs.Add(Runtime->GetBindingID());
			}
		}

		FString Header;

		for (const UScriptableEdGraphNode* EdNode : SelectedEdNodes)
		{
			const UScriptableNode* Runtime = EdNode->GetRuntimeNode();
			if (!Runtime) continue;

			const FVector2f Offset(EdNode->NodePosX - Centroid.X, EdNode->NodePosY - Centroid.Y);
			Header += FString::Printf(TEXT("%s%s|%f|%f\n"), PositionHeaderPrefix, *Runtime->GetBindingID().ToString(EGuidFormats::DigitsWithHyphens), Offset.X, Offset.Y);
		}

		for (const FScriptableGraphConnection& Conn : Asset.Connections)
		{
			if (!SelectedRuntimeIDs.Contains(Conn.From.NodeID) || !SelectedRuntimeIDs.Contains(Conn.To.NodeID)) continue;

			Header += FString::Printf(TEXT("%s%s|%s|%s|%s\n"),
				ConnectionHeaderPrefix,
				*Conn.From.NodeID.ToString(EGuidFormats::DigitsWithHyphens),
				*Conn.From.PinName.ToString(),
				*Conn.To.NodeID.ToString(EGuidFormats::DigitsWithHyphens),
				*Conn.To.PinName.ToString());
		}

		return Header;
	}

	/** Splits the clipboard text into header (positions + connections) and the T3D body. */
	void ParseClipboardHeader(const FString& FullText, TMap<FGuid, FVector2f>& OutOffsets, TArray<FCopiedConnectionEntry>& OutConnections, FString& OutBody)
	{
		OutOffsets.Reset();
		OutConnections.Reset();
		OutBody.Reset();

		TArray<FString> Lines;
		FullText.ParseIntoArrayLines(Lines, /*bCullEmpty*/ false);

		FString Body;
		Body.Reserve(FullText.Len());

		for (const FString& Line : Lines)
		{
			if (Line.StartsWith(PositionHeaderPrefix))
			{
				const FString Payload = Line.Mid(FCString::Strlen(PositionHeaderPrefix));
				TArray<FString> Parts;
				Payload.ParseIntoArray(Parts, TEXT("|"));
				if (Parts.Num() == 3)
				{
					FGuid Guid;
					if (FGuid::Parse(Parts[0], Guid))
					{
						OutOffsets.Add(Guid, FVector2f(FCString::Atof(*Parts[1]), FCString::Atof(*Parts[2])));
					}
				}
			}
			else if (Line.StartsWith(ConnectionHeaderPrefix))
			{
				const FString Payload = Line.Mid(FCString::Strlen(ConnectionHeaderPrefix));
				TArray<FString> Parts;
				Payload.ParseIntoArray(Parts, TEXT("|"));
				if (Parts.Num() == 4)
				{
					FCopiedConnectionEntry Entry;
					if (FGuid::Parse(Parts[0], Entry.OldFromNodeID) && FGuid::Parse(Parts[2], Entry.OldToNodeID))
					{
						Entry.FromPinName = FName(*Parts[1]);
						Entry.ToPinName = FName(*Parts[3]);
						OutConnections.Add(Entry);
					}
				}
			}
			else
			{
				Body += Line;
				Body += TEXT("\n");
			}
		}

		OutBody = MoveTemp(Body);
	}
}

const FName FScriptableGraphEditor::GraphTabId(TEXT("ScriptableGraphEditor_Graph"));
const FName FScriptableGraphEditor::AssetDetailsTabId(TEXT("ScriptableGraphEditor_AssetDetails"));
const FName FScriptableGraphEditor::NodeDetailsTabId(TEXT("ScriptableGraphEditor_NodeDetails"));

static const FName ScriptableGraphEditorAppId(TEXT("ScriptableGraphEditorApp"));

void FScriptableGraphEditor::CreateEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, const TArray<UObject*>& InObjects)
{
	for (UObject* Object : InObjects)
	{
		if (UScriptableGraph* Graph = Cast<UScriptableGraph>(Object))
		{
			const TSharedRef<FScriptableGraphEditor> Editor = MakeShared<FScriptableGraphEditor>();
			Editor->Initialize(Mode, InitToolkitHost, Graph);
		}
	}
}

FScriptableGraphEditor::~FScriptableGraphEditor()
{
	FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(OnObjectPropertyChangedHandle);
}

void FScriptableGraphEditor::Initialize(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UScriptableGraph* InGraph)
{
	EditedGraph = InGraph;

	// Create the details views up-front so RegisterTabSpawners can hand them out.
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs ViewArgs;
	ViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	ViewArgs.bHideSelectionTip = true;

	AssetDetailsView = PropertyEditorModule.CreateDetailView(ViewArgs);
	NodeDetailsView = PropertyEditorModule.CreateDetailView(ViewArgs);

	if (AssetDetailsView.IsValid())
	{
		AssetDetailsView->SetObject(InGraph);
	}

	if (NodeDetailsView.IsValid())
	{
		NodeDetailsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateLambda([](const FPropertyAndParent& PropertyAndParent)
			{
				return PropertyAndParent.Property.GetMetaData(TEXT("Category")) != TEXT("Tick");
			}));
	}

	BindGraphCommands();

	OnObjectPropertyChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddSP(this, &FScriptableGraphEditor::OnRuntimeNodePropertyChanged);

	// Ensure the asset has a visual UEdGraph, then mirror its runtime Nodes onto it.
	InitEdGraph();
	ReconstructEdGraphFromAsset();

	// Default three-pane layout: details left | graph center | node details right.
	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("ScriptableGraphEditor_Layout_v1")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.2f)
				->AddTab(AssetDetailsTabId, ETabState::OpenedTab)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.6f)
				->AddTab(GraphTabId, ETabState::OpenedTab)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.2f)
				->AddTab(NodeDetailsTabId, ETabState::OpenedTab)
			)
		);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor(Mode, InitToolkitHost, ScriptableGraphEditorAppId, Layout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, InGraph);
}

FName FScriptableGraphEditor::GetToolkitFName() const
{
	return FName("ScriptableGraphEditor");
}

FText FScriptableGraphEditor::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "Scriptable Graph Editor");
}

FString FScriptableGraphEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "ScriptableGraph ").ToString();
}

FLinearColor FScriptableGraphEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor::White;
}

void FScriptableGraphEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	const TSharedRef<FWorkspaceItem> Category = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu", "Scriptable Graph Editor"));

	InTabManager->RegisterTabSpawner(GraphTabId, FOnSpawnTab::CreateSP(this, &FScriptableGraphEditor::SpawnTab_Graph))
		.SetDisplayName(LOCTEXT("GraphTab", "Graph"))
		.SetGroup(Category)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.EventGraph_16x"));

	InTabManager->RegisterTabSpawner(AssetDetailsTabId, FOnSpawnTab::CreateSP(this, &FScriptableGraphEditor::SpawnTab_AssetDetails))
		.SetDisplayName(LOCTEXT("AssetDetailsTab", "Graph Details"))
		.SetGroup(Category)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));

	InTabManager->RegisterTabSpawner(NodeDetailsTabId, FOnSpawnTab::CreateSP(this, &FScriptableGraphEditor::SpawnTab_NodeDetails))
		.SetDisplayName(LOCTEXT("NodeDetailsTab", "Node Details"))
		.SetGroup(Category)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Properties"));
}

void FScriptableGraphEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(GraphTabId);
	InTabManager->UnregisterTabSpawner(AssetDetailsTabId);
	InTabManager->UnregisterTabSpawner(NodeDetailsTabId);
}

TSharedRef<SDockTab> FScriptableGraphEditor::SpawnTab_Graph(const FSpawnTabArgs& Args)
{
	UEdGraph* EdGraph = EditedGraph.IsValid() ? EditedGraph->EdGraph : nullptr;

	SGraphEditor::FGraphEditorEvents InEvents;
	InEvents.OnCreateActionMenuAtLocation = SGraphEditor::FOnCreateActionMenuAtLocation::CreateSP(this, &FScriptableGraphEditor::OnCreateNodeMenu);
	InEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FScriptableGraphEditor::OnGraphSelectionChanged);

	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("ScriptableGraphAppearanceCornerText", "SCRIPTABLE GRAPH");

	SAssignNew(GraphEditorWidget, SGraphEditor)
		.AdditionalCommands(GetToolkitCommands())
		.GraphToEdit(EdGraph)
		.GraphEvents(InEvents)
		.Appearance(AppearanceInfo);

	return SNew(SDockTab)
		.Label(LOCTEXT("GraphTab", "Graph"))
		[
			GraphEditorWidget.ToSharedRef()
		];
}

TSharedRef<SDockTab> FScriptableGraphEditor::SpawnTab_AssetDetails(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(LOCTEXT("AssetDetailsTab", "Graph Details"))
		[
			AssetDetailsView.IsValid() ? AssetDetailsView.ToSharedRef() : SNullWidget::NullWidget
		];
}

TSharedRef<SDockTab> FScriptableGraphEditor::SpawnTab_NodeDetails(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(LOCTEXT("NodeDetailsTab", "Node Details"))
		[
			NodeDetailsView.IsValid() ? NodeDetailsView.ToSharedRef() : SNullWidget::NullWidget
		];
}

void FScriptableGraphEditor::InitEdGraph()
{
	UScriptableGraph* Graph = EditedGraph.Get();
	if (!Graph || Graph->EdGraph) return;

	UScriptableEdGraph* NewEdGraph = NewObject<UScriptableEdGraph>(Graph, UScriptableEdGraph::StaticClass(), NAME_None, RF_Transactional);
	NewEdGraph->Schema = UScriptableEdGraphSchema::StaticClass();
	Graph->EdGraph = NewEdGraph;
	Graph->Modify();
}

void FScriptableGraphEditor::ReconstructEdGraphFromAsset()
{
	UScriptableGraph* Graph = EditedGraph.Get();
	if (!Graph || !Graph->EdGraph) return;

	// Build a set of runtime nodes that already have a visual representation.
	TSet<UScriptableNode*> AlreadyVisualized;
	for (UEdGraphNode* EdNode : Graph->EdGraph->Nodes)
	{
		if (UScriptableEdGraphNode* SfEdNode = Cast<UScriptableEdGraphNode>(EdNode))
		{
			if (UScriptableNode* RuntimeNode = SfEdNode->GetRuntimeNode())
			{
				AlreadyVisualized.Add(RuntimeNode);
			}
		}
	}

	// Spawn ed-nodes for any runtime node not yet represented. The visual class chosen mirrors
	// the one picked by ScriptableGraphEditorHelpers when nodes are created from the picker.
	for (const TObjectPtr<UScriptableNode>& RuntimeNode : Graph->Nodes)
	{
		if (!RuntimeNode || AlreadyVisualized.Contains(RuntimeNode)) continue;

		UScriptableEdGraphNode* NewEdNode = nullptr;
		if (RuntimeNode->IsA<UScriptableNode_Entry>())
		{
			NewEdNode = NewObject<UScriptableEdGraphNode_Entry>(Graph->EdGraph, UScriptableEdGraphNode_Entry::StaticClass(), NAME_None, RF_Transactional);
		}
		else if (RuntimeNode->IsA<UScriptableNode_Task>())
		{
			NewEdNode = NewObject<UScriptableEdGraphNode_Task>(Graph->EdGraph, UScriptableEdGraphNode_Task::StaticClass(), NAME_None, RF_Transactional);
		}
		else
		{
			NewEdNode = NewObject<UScriptableEdGraphNode>(Graph->EdGraph, UScriptableEdGraphNode::StaticClass(), NAME_None, RF_Transactional);
		}

		if (NewEdNode)
		{
			NewEdNode->SetRuntimeNode(RuntimeNode);
			NewEdNode->CreateNewGuid();
			NewEdNode->AllocateDefaultPins();
			Graph->EdGraph->AddNode(NewEdNode, /*bUserAction*/ false, /*bSelectNewNode*/ false);
		}
	}

	// Rebuild a NodeID -> ed-node lookup including the freshly spawned ones, then re-link pins
	// according to the persisted FScriptableGraphConnection list.
	TMap<FGuid, UScriptableEdGraphNode*> EdNodeByID;
	for (UEdGraphNode* EdNode : Graph->EdGraph->Nodes)
	{
		if (UScriptableEdGraphNode* SfEdNode = Cast<UScriptableEdGraphNode>(EdNode))
		{
			if (UScriptableNode* RuntimeNode = SfEdNode->GetRuntimeNode())
			{
				EdNodeByID.Add(RuntimeNode->GetBindingID(), SfEdNode);
			}
		}
	}

	for (const FScriptableGraphConnection& Conn : Graph->Connections)
	{
		UScriptableEdGraphNode* FromNode = EdNodeByID.FindRef(Conn.From.NodeID);
		UScriptableEdGraphNode* ToNode = EdNodeByID.FindRef(Conn.To.NodeID);
		if (!FromNode || !ToNode) continue;

		UEdGraphPin* FromPin = FromNode->FindPin(Conn.From.PinName, EGPD_Output);
		UEdGraphPin* ToPin = ToNode->FindPin(Conn.To.PinName, EGPD_Input);
		if (!FromPin || !ToPin) continue;

		// Defensive: avoid double-linking if reconstruction runs more than once.
		if (FromPin->LinkedTo.Contains(ToPin)) continue;

		FromPin->MakeLinkTo(ToPin);
	}
}

FActionMenuContent FScriptableGraphEditor::OnCreateNodeMenu(UEdGraph* InGraph, const FVector2f& InNodePosition, const TArray<UEdGraphPin*>& InDraggedPins, bool bAutoExpand, SGraphEditor::FActionMenuClosed InOnMenuClosed)
{
	// Capture spawn context by value so the deferred picker callback can use it after the menu closes.
	const FVector2f CapturedLocation(InNodePosition);
	TArray<UEdGraphPin*> CapturedPins = InDraggedPins;

	FSimpleDelegate MenuClosedAdapter = FSimpleDelegate::CreateLambda([InOnMenuClosed]()
		{
			InOnMenuClosed.ExecuteIfBound();
		});

	TSharedRef<SScriptableTypeSelector> TypeSelector = SNew(SScriptableTypeSelector)
		.TitleText(LOCTEXT("ContextMenuTitle", "Select Node"))
		.BaseClass(UScriptableTask::StaticClass())
		.ClassCategoryMeta(ScriptableFrameworkEditor::MD_TaskCategory)
		.AdditionalBaseClass(UScriptableNode::StaticClass())
		.AdditionalClassCategoryMeta(ScriptableFrameworkEditor::MD_NodeCategory)
		.OnNodeTypePicked(SScriptableTypeSelector::FOnNodeTypePicked::CreateSP(this, &FScriptableGraphEditor::OnNodeMenuTypePicked, InGraph, CapturedLocation, CapturedPins))
		.OnPickerClosed(MenuClosedAdapter);

	return FActionMenuContent(TypeSelector, TypeSelector->GetWidgetToFocusOnOpen());
}

void FScriptableGraphEditor::OnNodeMenuTypePicked(const UStruct* InStruct, const FAssetData& InAssetData, UEdGraph* InGraph, FVector2f InLocation, TArray<UEdGraphPin*> InDraggedPins)
{
	UEdGraphPin* FromPin = (InDraggedPins.Num() > 0) ? InDraggedPins[0] : nullptr;

	// Asset picks are deferred: when content-browser assets like UScriptableActionAsset
	// are dropped, we'll wrap them in UScriptableTask_RunAsset. Not implemented yet.
	if (InAssetData.IsValid())
	{
		FSlateApplication::Get().DismissAllMenus();
		return;
	}

	const UClass* PickedClass = Cast<UClass>(InStruct);
	if (!PickedClass)
	{
		FSlateApplication::Get().DismissAllMenus();
		return;
	}

	UEdGraphNode* SpawnedNode = nullptr;
	if (PickedClass->IsChildOf(UScriptableTask::StaticClass()))
	{
		SpawnedNode = ScriptableGraphEditorHelpers::SpawnTaskNode(InGraph, const_cast<UClass*>(PickedClass), InLocation, FromPin, /*bSelectNewNode*/ true);
	}
	else if (PickedClass->IsChildOf(UScriptableNode::StaticClass()))
	{
		SpawnedNode = ScriptableGraphEditorHelpers::SpawnNativeNode(InGraph, const_cast<UClass*>(PickedClass), InLocation, FromPin, /*bSelectNewNode*/ true);
	}

	// Dismiss the menu now that the spawn has executed. The picker's destructor will fire
	// OnPickerClosed → InOnMenuClosed afterwards, and SGraphPanel cleans up its drag-off state.
	FSlateApplication::Get().DismissAllMenus();

	if (SpawnedNode && GraphEditorWidget.IsValid())
	{
		GraphEditorWidget->ClearSelectionSet();
		GraphEditorWidget->SetNodeSelection(SpawnedNode, true);
	}
}

// ---------------------------------------------------------------------------------------------
// Keyboard commands
// ---------------------------------------------------------------------------------------------

void FScriptableGraphEditor::BindGraphCommands()
{
	const TSharedRef<FUICommandList> Commands = GetToolkitCommands();
	const FGenericCommands& Generic = FGenericCommands::Get();

	Commands->MapAction(Generic.Delete,
		FExecuteAction::CreateSP(this, &FScriptableGraphEditor::OnDeleteSelected),
		FCanExecuteAction::CreateSP(this, &FScriptableGraphEditor::CanDelete));

	Commands->MapAction(Generic.Copy,
		FExecuteAction::CreateSP(this, &FScriptableGraphEditor::OnCopySelected),
		FCanExecuteAction::CreateSP(this, &FScriptableGraphEditor::CanCopy));

	Commands->MapAction(Generic.Cut,
		FExecuteAction::CreateSP(this, &FScriptableGraphEditor::OnCutSelected),
		FCanExecuteAction::CreateSP(this, &FScriptableGraphEditor::CanCut));

	Commands->MapAction(Generic.Paste,
		FExecuteAction::CreateSP(this, &FScriptableGraphEditor::OnPasteAtCursor),
		FCanExecuteAction::CreateSP(this, &FScriptableGraphEditor::CanPaste));

	Commands->MapAction(Generic.Duplicate,
		FExecuteAction::CreateSP(this, &FScriptableGraphEditor::OnDuplicateSelected),
		FCanExecuteAction::CreateSP(this, &FScriptableGraphEditor::CanDuplicate));

	Commands->MapAction(Generic.SelectAll,
		FExecuteAction::CreateSP(this, &FScriptableGraphEditor::OnSelectAllNodes),
		FCanExecuteAction::CreateSP(this, &FScriptableGraphEditor::CanSelectAll));

	// Undo/Redo: forward to GEditor's transaction stack. FAssetEditorToolkit already provides the
	// callbacks; map them through so Ctrl+Z / Ctrl+Y / Ctrl+Shift+Z hit the same path while focus
	// is inside the SGraphEditor widget.
	Commands->MapAction(Generic.Undo,
		FExecuteAction::CreateLambda([]() { if (GEditor) GEditor->UndoTransaction(); }));

	Commands->MapAction(Generic.Redo,
		FExecuteAction::CreateLambda([]() { if (GEditor) GEditor->RedoTransaction(); }));
}

bool FScriptableGraphEditor::HasAnyNodesSelected() const
{
	return GraphEditorWidget.IsValid() && GraphEditorWidget->GetSelectedNodes().Num() > 0;
}

bool FScriptableGraphEditor::HasAnyDeletableSelected() const
{
	if (!GraphEditorWidget.IsValid()) return false;

	for (UObject* Selected : GraphEditorWidget->GetSelectedNodes())
	{
		if (const UEdGraphNode* Node = Cast<UEdGraphNode>(Selected))
		{
			if (Node->CanUserDeleteNode()) return true;
		}
	}
	return false;
}

bool FScriptableGraphEditor::HasAnyCopyableSelected() const
{
	if (!GraphEditorWidget.IsValid()) return false;

	for (UObject* Selected : GraphEditorWidget->GetSelectedNodes())
	{
		if (const UEdGraphNode* Node = Cast<UEdGraphNode>(Selected))
		{
			if (Node->CanDuplicateNode()) return true;
		}
	}
	return false;
}

bool FScriptableGraphEditor::CanPaste() const
{
	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);
	if (ClipboardContent.IsEmpty()) return false;

	// Mirrors FScriptableObjectCustomization::OnPasteNode: the clipboard text must parse into a UScriptableNode.
	TKzObjectTextFactory<UScriptableNode> Factory;
	return Factory.CanCreateObjectsFromText(ClipboardContent);
}

void FScriptableGraphEditor::OnDeleteSelected()
{
	if (!GraphEditorWidget.IsValid() || !EditedGraph.IsValid() || !EditedGraph->EdGraph) return;

	const FScopedTransaction Transaction(LOCTEXT("DeleteSelectedNodes", "Delete Selected Nodes"));

	UScriptableGraph* GraphAsset = EditedGraph.Get();
	UEdGraph* EdGraph = GraphAsset->EdGraph;

	GraphAsset->Modify();
	EdGraph->Modify();

	const FGraphPanelSelectionSet Selection = GraphEditorWidget->GetSelectedNodes();
	GraphEditorWidget->ClearSelectionSet();

	for (UObject* Selected : Selection)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(Selected);
		if (!Node || !Node->CanUserDeleteNode()) continue;

		// DestroyNode on UScriptableEdGraphNode also strips the runtime node and its wires
		// from the asset (Entry is exempted there).
		Node->Modify();
		Node->DestroyNode();
	}

	GraphEditorWidget->NotifyGraphChanged();
}

void FScriptableGraphEditor::OnCopySelected()
{
	if (!GraphEditorWidget.IsValid() || !EditedGraph.IsValid()) return;

	// Single-node copies skip the header entirely so the clipboard stays interoperable with the
	// details-view clipboard (FScriptableObjectCustomization::OnCopyNode).
	TArray<UScriptableEdGraphNode*> SelectedEdNodes;
	TArray<UObject*> RuntimeNodes;
	for (UObject* Selected : GraphEditorWidget->GetSelectedNodes())
	{
		UScriptableEdGraphNode* SfNode = Cast<UScriptableEdGraphNode>(Selected);
		if (!SfNode || !SfNode->CanDuplicateNode()) continue;

		if (UScriptableNode* RuntimeNode = SfNode->GetRuntimeNode())
		{
			SelectedEdNodes.Add(SfNode);
			RuntimeNodes.Add(RuntimeNode);
		}
	}

	if (RuntimeNodes.IsEmpty()) return;

	FKzClipboardUtils::CopyObjectsToClipboard(RuntimeNodes);

	if (SelectedEdNodes.Num() > 1)
	{
		// Prepend our graph-specific header. T3D treats lines starting with ';' as comments, so
		// builds opening this clipboard outside the graph editor still parse correctly.
		FString CurrentClipboard;
		FPlatformApplicationMisc::ClipboardPaste(CurrentClipboard);

		const FString Header = BuildClipboardHeader(SelectedEdNodes, *EditedGraph.Get());
		const FString WithHeader = Header + CurrentClipboard;
		FPlatformApplicationMisc::ClipboardCopy(*WithHeader);
	}
}

void FScriptableGraphEditor::OnCutSelected()
{
	// Cut = Copy first eligible + Delete all eligible. Single transaction so undo is atomic.
	const FScopedTransaction Transaction(LOCTEXT("CutSelectedNodes", "Cut Selected Nodes"));
	OnCopySelected();
	OnDeleteSelected();
}

void FScriptableGraphEditor::OnPasteAtCursor()
{
	if (!GraphEditorWidget.IsValid() || !EditedGraph.IsValid() || !EditedGraph->EdGraph) return;

	UScriptableGraph* GraphAsset = EditedGraph.Get();
	UEdGraph* EdGraph = GraphAsset->EdGraph;

	// Pull the raw clipboard text so we can peel off our header (positions + connections) before
	// handing the T3D body to the factory. Skipping straight to ClipboardPaste keeps single-node
	// clipboards (from details views) working: no header → empty offsets/connections → cascade fallback.
	FString FullClipboardText;
	FPlatformApplicationMisc::ClipboardPaste(FullClipboardText);
	if (FullClipboardText.IsEmpty()) return;

	TMap<FGuid, FVector2f> OldNodeOffsets;
	TArray<FCopiedConnectionEntry> CopiedConnections;
	FString T3DBody;
	ParseClipboardHeader(FullClipboardText, OldNodeOffsets, CopiedConnections, T3DBody);

	// Run the body through the factory directly (parallel of FKzClipboardUtils::PasteObjectsFromClipboard,
	// but the buffer comes from the stripped text rather than the raw clipboard).
	TArray<UScriptableNode*> NewRuntimeNodes;
	{
		TKzObjectTextFactory<UScriptableNode> Factory;
		if (Factory.CanCreateObjectsFromText(T3DBody))
		{
			Factory.ProcessBuffer(GraphAsset, RF_Transactional, T3DBody);
			NewRuntimeNodes = MoveTemp(Factory.CreatedObjects);
		}
	}
	if (NewRuntimeNodes.IsEmpty()) return;

	// Entry is unique per graph (auto-repaired by EnsureEntryNode). Strip any Entry from the
	// paste payload and warn — never block the whole paste because of an Entry sneaking in.
	bool bRefusedAnEntry = false;
	NewRuntimeNodes.RemoveAll([&bRefusedAnEntry](UScriptableNode* Node)
		{
			if (Node && Node->IsA<UScriptableNode_Entry>())
			{
				bRefusedAnEntry = true;
				Node->ClearFlags(RF_Transactional);
				Node->MarkAsGarbage();
				return true;
			}
			return false;
		});

	if (bRefusedAnEntry)
	{
		FNotificationInfo Info(LOCTEXT("PasteEntryRefused", "Skipped Entry node(s) — graphs have exactly one."));
		Info.ExpireDuration = 3.0f;
		Info.Image = FAppStyle::GetBrush("Icons.Warning");
		FSlateNotificationManager::Get().AddNotification(Info);
	}

	if (NewRuntimeNodes.IsEmpty()) return;

	const FScopedTransaction Transaction(LOCTEXT("PasteNodes", "Paste Nodes"));
	GraphAsset->Modify();
	EdGraph->Modify();

	const FVector2f BasePasteLocation = GraphEditorWidget->GetPasteLocation2f();
	const FVector2f CascadeStep(20.f, 20.f); // Fallback for clipboards without our header.

	// Build OldID -> new runtime map as we paste, so we can rewire connections afterwards.
	// Order in NewRuntimeNodes matches the order objects were exported, which matches the order
	// of ;SF_POS lines in the header, but we cannot rely on indices alone if Entry was dropped
	// mid-loop above. Walking both lists in parallel works because BindClipboardHeader iterates
	// SelectedEdNodes in the same order CopyObjectsToClipboard iterates RuntimeNodes.
	TArray<FGuid> OldIDsInOrder;
	OldIDsInOrder.Reserve(OldNodeOffsets.Num());
	{
		// Re-parse the header in line order so OldIDsInOrder matches the body's export order.
		TArray<FString> Lines;
		FullClipboardText.ParseIntoArrayLines(Lines, /*bCullEmpty*/ false);
		for (const FString& Line : Lines)
		{
			if (!Line.StartsWith(PositionHeaderPrefix)) continue;
			TArray<FString> Parts;
			Line.Mid(FCString::Strlen(PositionHeaderPrefix)).ParseIntoArray(Parts, TEXT("|"));
			FGuid Guid;
			if (Parts.Num() >= 1 && FGuid::Parse(Parts[0], Guid))
			{
				OldIDsInOrder.Add(Guid);
			}
		}
	}

	TMap<FGuid, UScriptableNode*> OldIDToNewRuntime;
	OldIDToNewRuntime.Reserve(NewRuntimeNodes.Num());

	GraphEditorWidget->ClearSelectionSet();

	for (int32 i = 0; i < NewRuntimeNodes.Num(); ++i)
	{
		UScriptableNode* NewRuntimeNode = NewRuntimeNodes[i];

		// Patch bindings: the deserialized runtime got a fresh BindingID via PostEditImport, but its
		// PropertyBindings still carry the OLD BindingID in TargetPath. Re-stamp them so the bindings
		// target the new instance. Same fix as FScriptableObjectCustomization::OnPasteNode.
		const FGuid NewGuid = NewRuntimeNode->GetBindingID();
		for (FScriptablePropertyBinding& Binding : NewRuntimeNode->GetPropertyBindings().Bindings)
		{
			Binding.TargetPath.SetStructID(NewGuid);
		}

		GraphAsset->Nodes.Add(NewRuntimeNode);

		// Position: use the header offset if available, otherwise fall back to cascade.
		FVector2f Location;
		if (OldIDsInOrder.IsValidIndex(i))
		{
			const FGuid OldID = OldIDsInOrder[i];
			OldIDToNewRuntime.Add(OldID, NewRuntimeNode);

			if (const FVector2f* Offset = OldNodeOffsets.Find(OldID))
			{
				Location = BasePasteLocation + (*Offset);
			}
			else
			{
				Location = BasePasteLocation + CascadeStep * static_cast<float>(i);
			}
		}
		else
		{
			Location = BasePasteLocation + CascadeStep * static_cast<float>(i);
		}

		UScriptableEdGraphNode* NewEdNode = ScriptableGraphEditorHelpers::SpawnEdNodeForRuntime(EdGraph, NewRuntimeNode, Location);

		if (NewEdNode)
		{
			GraphEditorWidget->SetNodeSelection(NewEdNode, true);
		}
	}

	// Recreate connections between freshly pasted nodes. Connections whose endpoints aren't both
	// in OldIDToNewRuntime (e.g. one side was an Entry we stripped) are silently dropped.
	for (const FCopiedConnectionEntry& Entry : CopiedConnections)
	{
		UScriptableNode** NewFromRuntime = OldIDToNewRuntime.Find(Entry.OldFromNodeID);
		UScriptableNode** NewToRuntime = OldIDToNewRuntime.Find(Entry.OldToNodeID);
		if (!NewFromRuntime || !NewToRuntime) continue;

		// Persist in the asset.
		FScriptableGraphConnection NewConn;
		NewConn.From.NodeID = (*NewFromRuntime)->GetBindingID();
		NewConn.From.PinName = Entry.FromPinName;
		NewConn.To.NodeID = (*NewToRuntime)->GetBindingID();
		NewConn.To.PinName = Entry.ToPinName;
		GraphAsset->Connections.Add(NewConn);

		// Mirror on the visual graph: find the ed-nodes wrapping these runtimes and link their pins.
		UScriptableEdGraphNode* FromEdNode = nullptr;
		UScriptableEdGraphNode* ToEdNode = nullptr;
		for (UEdGraphNode* EdNode : EdGraph->Nodes)
		{
			if (UScriptableEdGraphNode* SfEd = Cast<UScriptableEdGraphNode>(EdNode))
			{
				if (SfEd->GetRuntimeNode() == *NewFromRuntime) FromEdNode = SfEd;
				else if (SfEd->GetRuntimeNode() == *NewToRuntime) ToEdNode = SfEd;
				if (FromEdNode && ToEdNode) break;
			}
		}

		if (FromEdNode && ToEdNode)
		{
			UEdGraphPin* FromPin = FromEdNode->FindPin(Entry.FromPinName, EGPD_Output);
			UEdGraphPin* ToPin = ToEdNode->FindPin(Entry.ToPinName, EGPD_Input);
			if (FromPin && ToPin)
			{
				FromPin->MakeLinkTo(ToPin);
			}
		}
	}

	GraphEditorWidget->NotifyGraphChanged();
}

void FScriptableGraphEditor::OnDuplicateSelected()
{
	// Duplicate = Copy + Paste, single transaction.
	const FScopedTransaction Transaction(LOCTEXT("DuplicateNodes", "Duplicate Nodes"));
	OnCopySelected();
	OnPasteAtCursor();
}

void FScriptableGraphEditor::OnSelectAllNodes()
{
	if (GraphEditorWidget.IsValid())
	{
		GraphEditorWidget->SelectAllNodes();
	}
}

void FScriptableGraphEditor::OnRuntimeNodePropertyChanged(UObject* InObject, FPropertyChangedEvent& InEvent)
{
	UScriptableGraph* Graph = EditedGraph.Get();
	if (!Graph || !Graph->EdGraph || !InObject) return;

	// Walk up the outer chain of the changed object until we find a UScriptableNode owned by the
	// edited graph. This catches both direct edits on the wrapper and edits on the inner task
	// (whose outer is the wrapper, whose outer is the graph asset).
	for (UObject* Cursor = InObject; Cursor; Cursor = Cursor->GetOuter())
	{
		UScriptableNode* Node = Cast<UScriptableNode>(Cursor);
		if (!Node || Node->GetOuter() != Graph) continue;

		for (UEdGraphNode* EdNode : Graph->EdGraph->Nodes)
		{
			UScriptableEdGraphNode* SfEd = Cast<UScriptableEdGraphNode>(EdNode);
			if (SfEd && SfEd->GetRuntimeNode() == Node)
			{
				SfEd->ReconstructNode();
				return;
			}
		}
		return;
	}
}

void FScriptableGraphEditor::OnGraphSelectionChanged(const FGraphPanelSelectionSet& NewSelection)
{
	if (!NodeDetailsView.IsValid()) return;

	// Single-selection only: anything else (empty or multi) clears the panel. Bindings make
	// multi-edit semantically dubious (each node has its own context references), so we don't
	// even try to present a homogeneous array.
	if (NewSelection.Num() != 1)
	{
		NodeDetailsView->SetObject(nullptr);
		return;
	}

	UObject* Selected = *NewSelection.CreateConstIterator();
	UScriptableEdGraphNode* SfEdNode = Cast<UScriptableEdGraphNode>(Selected);
	if (!SfEdNode)
	{
		NodeDetailsView->SetObject(nullptr);
		return;
	}

	UScriptableNode* RuntimeNode = SfEdNode->GetRuntimeNode();
	if (!RuntimeNode)
	{
		NodeDetailsView->SetObject(nullptr);
		return;
	}

	NodeDetailsView->SetObject(RuntimeNode);
}

#undef LOCTEXT_NAMESPACE
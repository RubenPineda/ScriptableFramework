// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/Editor/ScriptableGraphEditor.h"
#include "ScriptableFrameworkEd/Graph/Schema/ScriptableEdGraph.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNodeRegistry.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphSchema.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode_Native.h"
#include "ScriptableFrameworkEd/Graph/ScriptableGraphCommands.h"
#include "ScriptableNodes/ScriptableNode_Entry.h"
#include "ScriptableNodes/ScriptableNode_Sequence.h"
#include "ScriptableNodes/ScriptableNode_AND.h"
#include "ScriptableNodes/ScriptableNode_Branch.h"
#include "ScriptableNodes/ScriptableNode_Task.h"
#include "ScriptableFrameworkEd/Graph/ScriptableGraphEditorHelpers.h"
#include "ScriptableFrameworkEd/Customization/Widgets/SScriptableTypePicker.h"
#include "ScriptableFrameworkEditorHelpers.h"
#include "ScriptableNodes/ScriptableGraph.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableTasks/ScriptableTask.h"
#include "ScriptableTasks/ScriptableActionAsset.h"

#include "GraphEditor.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraphNode_Comment.h"
#include "GraphEditorActions.h"
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
#include "UObject/Package.h"
#include "Widgets/SKzValidationPanel.h"
#include "Validation/KzAssetValidationUtils.h"
#include "Core/KzValidationTypes.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ScriptableNodes/ScriptableNode_ReceiveEvent.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/SBoxPanel.h"

#define LOCTEXT_NAMESPACE "ScriptableGraphEditor"

namespace
{
	// Clipboard-header prefixes. Start with ';' so the engine's T3D text factory skips them as comments.
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

	/** Compares the ed-node's current pin set (names + directions) against what the runtime node declares. Returns true if anything differs, prompting a full Reconstruct; false if they're already in sync, in which case we leave the ed-node alone to avoid spurious dirty-flagging at editor open time. */
	bool ArePinsOutOfSyncWithRuntime(const UScriptableEdGraphNode* EdNode, const UScriptableNode* RuntimeNode)
	{
		if (!EdNode || !RuntimeNode) return false;

		const TArray<FName> RuntimeInputs = RuntimeNode->GetInputPins();
		const TArray<FName> RuntimeOutputs = RuntimeNode->GetOutputPins();

		int32 InputCount = 0;
		int32 OutputCount = 0;
		for (const UEdGraphPin* Pin : EdNode->Pins)
		{
			if (!Pin) continue;
			if (Pin->Direction == EGPD_Input)
			{
				if (!RuntimeInputs.Contains(Pin->PinName)) return true;
				++InputCount;
			}
			else
			{
				if (!RuntimeOutputs.Contains(Pin->PinName)) return true;
				++OutputCount;
			}
		}

		return InputCount != RuntimeInputs.Num() || OutputCount != RuntimeOutputs.Num();
	}
}

const FName FScriptableGraphEditor::GraphTabId(TEXT("ScriptableGraphEditor_Graph"));
const FName FScriptableGraphEditor::AssetDetailsTabId(TEXT("ScriptableGraphEditor_AssetDetails"));
const FName FScriptableGraphEditor::NodeDetailsTabId(TEXT("ScriptableGraphEditor_NodeDetails"));
const FName FScriptableGraphEditor::PaletteTabId(TEXT("ScriptableGraphEditor_Palette"));
const FName FScriptableGraphEditor::ValidationTabId(TEXT("ScriptableGraphEditor_Validation"));
const FName FScriptableGraphEditor::SearchTabId(TEXT("ScriptableGraphEditor_Search"));

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
	FCoreUObjectDelegates::OnObjectTransacted.Remove(OnObjectTransactedHandle);
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

	OnObjectTransactedHandle = FCoreUObjectDelegates::OnObjectTransacted.AddSP(
		StaticCastSharedRef<FScriptableGraphEditor>(AsShared()),
		&FScriptableGraphEditor::OnObjectTransacted);

	// Ensure the asset has a visual UEdGraph, then mirror its runtime Nodes onto it.
	InitEdGraph();
	ReconstructEdGraphFromAsset();

	// Default three-pane layout: details left | graph center | node details right.
	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("ScriptableGraphEditor_Layout_v4")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Horizontal)
			->Split
			(
				// Left column: Asset Details on top, Palette below — same split BP uses.
				FTabManager::NewSplitter()
				->SetSizeCoefficient(0.2f)
				->SetOrientation(Orient_Vertical)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.6f)
					->AddTab(AssetDetailsTabId, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.4f)
					->AddTab(PaletteTabId, ETabState::OpenedTab)
				)
			)
			->Split
			(
				FTabManager::NewSplitter()
				->SetSizeCoefficient(0.6f)
				->SetOrientation(Orient_Vertical)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.75f)
					->AddTab(GraphTabId, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.25f)
					->AddTab(SearchTabId, ETabState::ClosedTab)
				)
			)
			->Split
			(
				FTabManager::NewSplitter()
				->SetSizeCoefficient(0.2f)
				->SetOrientation(Orient_Vertical)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.6f)
					->AddTab(NodeDetailsTabId, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.4f)
					->AddTab(ValidationTabId, ETabState::OpenedTab)
				)
			)
		);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor(Mode, InitToolkitHost, ScriptableGraphEditorAppId, Layout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, InGraph);

	ExtendToolbar();
	RegenerateMenusAndToolbars();
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

	InTabManager->RegisterTabSpawner(PaletteTabId, FOnSpawnTab::CreateSP(this, &FScriptableGraphEditor::SpawnTab_Palette))
		.SetDisplayName(LOCTEXT("PaletteTabLabel", "Palette"))
		.SetGroup(Category)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Kismet.Tabs.Palette"));

	InTabManager->RegisterTabSpawner(ValidationTabId, FOnSpawnTab::CreateSP(this, &FScriptableGraphEditor::SpawnTab_Validation))
		.SetDisplayName(LOCTEXT("ValidationTabLabel", "Validation"))
		.SetGroup(Category)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.WarningWithColor"));

	InTabManager->RegisterTabSpawner(SearchTabId, FOnSpawnTab::CreateSP(this, &FScriptableGraphEditor::SpawnTab_Search))
		.SetDisplayName(LOCTEXT("SearchTabLabel", "Search"))
		.SetGroup(Category)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Search"));
}

void FScriptableGraphEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(GraphTabId);
	InTabManager->UnregisterTabSpawner(AssetDetailsTabId);
	InTabManager->UnregisterTabSpawner(NodeDetailsTabId);
	InTabManager->UnregisterTabSpawner(PaletteTabId);
	InTabManager->UnregisterTabSpawner(ValidationTabId);
	InTabManager->UnregisterTabSpawner(SearchTabId);
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

TSharedRef<SDockTab> FScriptableGraphEditor::SpawnTab_Palette(const FSpawnTabArgs& Args)
{
	TSharedRef<SScriptableTypeSelector> Selector = SNew(SScriptableTypeSelector)
		.BaseClass(UScriptableTask::StaticClass())
		.ClassCategoryMeta(ScriptableFrameworkEditor::MD_TaskCategory)
		.AdditionalBaseClass(UScriptableNode::StaticClass())
		.AdditionalClassCategoryMeta(ScriptableFrameworkEditor::MD_NodeCategory)
		.BaseClassRootCategory(LOCTEXT("PaletteScriptableTasks", "Scriptable Tasks"))
		.AdditionalBaseClassRootCategory(LOCTEXT("PaletteNativeNodes", "Native Nodes"))
		.EnableDragOut(true);

	return SNew(SDockTab)
		.Label(LOCTEXT("PaletteTabLabel", "Palette"))
		[
			Selector
		];
}

void FScriptableGraphEditor::InitEdGraph()
{
	UScriptableGraph* Graph = EditedGraph.Get();
	if (!Graph || Graph->EdGraph) return;

	UScriptableEdGraph* NewEdGraph = NewObject<UScriptableEdGraph>(Graph, UScriptableEdGraph::StaticClass(), NAME_None, RF_Transactional);
	NewEdGraph->Schema = UScriptableEdGraphSchema::StaticClass();
	Graph->EdGraph = NewEdGraph;
}

void FScriptableGraphEditor::ReconstructEdGraphFromAsset()
{
	UScriptableGraph* Graph = EditedGraph.Get();
	if (!Graph || !Graph->EdGraph) return;

	// Rebuilding the visual graph is a derived-view sync, not a user edit, so it must not dirty the
	// asset. AddNode / MakeLinkTo call Modify() internally, so snapshot the package dirty state and
	// restore it at the end. Callers that made a real edit before us leave it dirty (snapshot is true).
	UPackage* Package = Graph->GetPackage();
	const bool bWasDirty = Package && Package->IsDirty();

	TSet<UScriptableNode*> AlreadyVisualized;
	TMap<FGuid, UScriptableEdGraphNode*> EdNodeByID;

	// 1. Sync visual pins and wipe visual links to prepare for a clean rebuild from the asset.
	for (UEdGraphNode* EdNode : Graph->EdGraph->Nodes)
	{
		if (UScriptableEdGraphNode* SfEdNode = Cast<UScriptableEdGraphNode>(EdNode))
		{
			if (UScriptableNode* RuntimeNode = SfEdNode->GetRuntimeNode())
			{
				// Only reconstruct pins if they're actually out of sync with the runtime.
				if (ArePinsOutOfSyncWithRuntime(SfEdNode, RuntimeNode))
				{
					SfEdNode->ReconstructNode();
				}

				AlreadyVisualized.Add(RuntimeNode);
				EdNodeByID.Add(RuntimeNode->GetBindingID(), SfEdNode);
			}

			// Manually break all visual links without triggering the schema
			for (UEdGraphPin* Pin : SfEdNode->Pins)
			{
				for (UEdGraphPin* Linked : Pin->LinkedTo)
				{
					if (Linked) Linked->LinkedTo.Remove(Pin);
				}
				Pin->LinkedTo.Empty();
			}
		}
	}

	// 2. Spawn ed-nodes for any runtime node not yet represented.
	for (const TObjectPtr<UScriptableNode>& RuntimeNode : Graph->Nodes)
	{
		if (!RuntimeNode || AlreadyVisualized.Contains(RuntimeNode)) continue;

		UClass* EdNodeClass = FScriptableEdGraphNodeRegistry::FindEdNodeClassFor(RuntimeNode);
		if (!EdNodeClass) EdNodeClass = UScriptableEdGraphNode_Native::StaticClass();

		UScriptableEdGraphNode* NewEdNode = NewObject<UScriptableEdGraphNode>(Graph->EdGraph, EdNodeClass, NAME_None, RF_Transactional);

		if (NewEdNode)
		{
			NewEdNode->SetRuntimeNode(RuntimeNode);
			NewEdNode->CreateNewGuid();
			NewEdNode->AllocateDefaultPins();
			Graph->EdGraph->AddNode(NewEdNode, /*bUserAction*/ false, /*bSelectNewNode*/ false);

			EdNodeByID.Add(RuntimeNode->GetBindingID(), NewEdNode);
		}
	}

	// 3. Re-link pins according to the persisted FScriptableGraphConnection list.
	for (const FScriptableGraphConnection& Conn : Graph->Connections)
	{
		UScriptableEdGraphNode* FromNode = EdNodeByID.FindRef(Conn.From.NodeID);
		UScriptableEdGraphNode* ToNode = EdNodeByID.FindRef(Conn.To.NodeID);
		if (!FromNode || !ToNode) continue;

		UEdGraphPin* FromPin = FromNode->FindPin(Conn.From.PinName, EGPD_Output);
		UEdGraphPin* ToPin = ToNode->FindPin(Conn.To.PinName, EGPD_Input);
		if (!FromPin || !ToPin) continue;

		FromPin->MakeLinkTo(ToPin);
	}

	Graph->EdGraph->NotifyGraphChanged();

	// Drop the dirtiness introduced purely by rebuilding the view (see snapshot above).
	if (Package && !bWasDirty)
	{
		Package->SetDirtyFlag(false);
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
		.BaseClassRootCategory(LOCTEXT("PickerScriptableTasks", "Scriptable Tasks"))
		.AdditionalBaseClassRootCategory(LOCTEXT("PickerNativeNodes", "Native Nodes"))
		.OnNodeTypePicked(SScriptableTypeSelector::FOnNodeTypePicked::CreateSP(this, &FScriptableGraphEditor::OnNodeMenuTypePicked, InGraph, CapturedLocation, CapturedPins))
		.OnPickerClosed(MenuClosedAdapter);

	return FActionMenuContent(TypeSelector, TypeSelector->GetWidgetToFocusOnOpen());
}

void FScriptableGraphEditor::OnNodeMenuTypePicked(const UStruct* InStruct, const FAssetData& InAssetData, UEdGraph* InGraph, FVector2f InLocation, TArray<UEdGraphPin*> InDraggedPins)
{
	UEdGraphPin* FromPin = (InDraggedPins.Num() > 0) ? InDraggedPins[0] : nullptr;

	UEdGraphNode* SpawnedNode = nullptr;

	// Asset path: a UScriptableActionAsset spawns a Task wrapper around UScriptableTask_RunAsset
	// pointing at the picked asset. From there it behaves like any other task.
	if (InAssetData.IsValid())
	{
		UScriptableActionAsset* ActionAsset = Cast<UScriptableActionAsset>(InAssetData.GetAsset());
		if (ActionAsset)
		{
			SpawnedNode = ScriptableGraphEditorHelpers::SpawnTaskNode(InGraph, UScriptableTask_RunAsset::StaticClass(), InLocation, FromPin, /*bSelectNewNode*/ true);

			if (UScriptableEdGraphNode* SfEdNode = Cast<UScriptableEdGraphNode>(SpawnedNode))
			{
				if (UScriptableNode_Task* WrapperRuntime = Cast<UScriptableNode_Task>(SfEdNode->GetRuntimeNode()))
				{
					if (UScriptableTask_RunAsset* RunAssetTask = Cast<UScriptableTask_RunAsset>(WrapperRuntime->Task))
					{
						RunAssetTask->Modify();
						RunAssetTask->Asset = ActionAsset;

						// Refresh the node title so the canvas shows the asset name immediately rather
						// than the generic "Run Asset" default.
						SfEdNode->ReconstructNode();
					}
				}
			}
		}
	}
	else
	{
		const UClass* PickedClass = Cast<UClass>(InStruct);
		if (PickedClass)
		{
			if (PickedClass->IsChildOf(UScriptableTask::StaticClass()))
			{
				SpawnedNode = ScriptableGraphEditorHelpers::SpawnTaskNode(InGraph, const_cast<UClass*>(PickedClass), InLocation, FromPin, /*bSelectNewNode*/ true);
			}
			else if (PickedClass->IsChildOf(UScriptableNode::StaticClass()))
			{
				SpawnedNode = ScriptableGraphEditorHelpers::SpawnNativeNode(InGraph, const_cast<UClass*>(PickedClass), InLocation, FromPin, /*bSelectNewNode*/ true);
			}
		}
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

void FScriptableGraphEditor::OnPaletteTypePicked(const UStruct* InStruct, const FAssetData& InAssetData)
{
	if (!GraphEditorWidget.IsValid()) return;

	UEdGraph* Graph = GraphEditorWidget->GetCurrentGraph();
	if (!Graph) return;

	const FVector2f Location = GraphEditorWidget->GetPasteLocation2f();

	// Reuse the same dispatch the right-click menu uses. Pass an empty pin list since there's no
	// drag-off context to wire from.
	OnNodeMenuTypePicked(InStruct, InAssetData, Graph, Location, TArray<UEdGraphPin*>());
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

	Commands->MapAction(FGraphEditorCommands::Get().DeleteAndReconnectNodes,
		FExecuteAction::CreateSP(this, &FScriptableGraphEditor::OnDeleteAndReconnectNodes),
		FCanExecuteAction::CreateSP(this, &FScriptableGraphEditor::CanDeleteAndReconnectNodes));

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

	Commands->MapAction(FGraphEditorCommands::Get().CreateComment,
		FExecuteAction::CreateSP(this, &FScriptableGraphEditor::OnCreateComment));

	// Undo/Redo: forward to GEditor's transaction stack so the shortcuts work while focus is in the graph widget.
	Commands->MapAction(Generic.Undo,
		FExecuteAction::CreateLambda([]() { if (GEditor) GEditor->UndoTransaction(); }));

	Commands->MapAction(Generic.Redo,
		FExecuteAction::CreateLambda([]() { if (GEditor) GEditor->RedoTransaction(); }));

	// Pin context: Remove pin. Resolved against this command list (SGraphEditor was built with AdditionalCommands = GetToolkitCommands()).
	Commands->MapAction(FScriptableGraphCommands::Get().RemoveSequencePin,
		FExecuteAction::CreateSP(this, &FScriptableGraphEditor::OnRemoveSequencePin),
		FCanExecuteAction::CreateSP(this, &FScriptableGraphEditor::CanRemoveSequencePin));

	Commands->MapAction(FScriptableGraphCommands::Get().RemoveANDPin,
		FExecuteAction::CreateSP(this, &FScriptableGraphEditor::OnRemoveANDPin),
		FCanExecuteAction::CreateSP(this, &FScriptableGraphEditor::CanRemoveANDPin));
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

bool FScriptableGraphEditor::CanDeleteAndReconnectNodes() const
{
	if (!GraphEditorWidget.IsValid()) return false;
	return GraphEditorWidget->GetSelectedNodes().Num() > 0;
}

void FScriptableGraphEditor::OnDeleteAndReconnectNodes()
{
	if (!GraphEditorWidget.IsValid()) return;

	const FGraphPanelSelectionSet Selection = GraphEditorWidget->GetSelectedNodes();
	if (Selection.Num() == 0) return;

	UEdGraph* Graph = GraphEditorWidget->GetCurrentGraph();
	if (!Graph) return;

	const FScopedTransaction Transaction(LOCTEXT("DeleteAndReconnectTx", "Delete and Reconnect"));
	Graph->Modify();

	for (UObject* SelectedObj : Selection)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(SelectedObj);
		if (!Node || !Node->CanUserDeleteNode()) continue;

		for (UEdGraphPin* InputPin : Node->Pins)
		{
			if (!InputPin || InputPin->Direction != EGPD_Input) continue;

			for (UEdGraphPin* OutputPin : Node->Pins)
			{
				if (!OutputPin || OutputPin->Direction != EGPD_Output) continue;

				// Snapshot the peer arrays — we'll mutate LinkedTo while iterating otherwise.
				TArray<UEdGraphPin*> UpstreamPeers = InputPin->LinkedTo;
				TArray<UEdGraphPin*> DownstreamPeers = OutputPin->LinkedTo;

				for (UEdGraphPin* Up : UpstreamPeers)
				{
					for (UEdGraphPin* Down : DownstreamPeers)
					{
						if (!Up || !Down) continue;
						if (Up->GetOwningNode() == Node || Down->GetOwningNode() == Node) continue;

						// Route through the schema so Asset->Connections stays in sync (the schema
						// owns the persist-to-runtime step on TryCreateConnection).
						const UEdGraphSchema* Schema = Node->GetSchema();
						if (Schema)
						{
							Schema->TryCreateConnection(Up, Down);
						}
					}
				}
			}
		}

		Node->Modify();
		Node->DestroyNode();
	}

	Graph->NotifyGraphChanged();
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

	// Peel our header (positions + connections) off the raw clipboard before handing the T3D body to
	// the factory. Single-node clipboards (from details views) have no header → cascade fallback.
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

	// Build OldID -> new runtime map as we paste, to rewire connections afterwards. Walk NewRuntimeNodes
	// and the ;SF_POS order in parallel (same order CopyObjectsToClipboard exported them).
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

		// The deserialized runtime got a fresh BindingID, but its PropertyBindings still carry the
		// old one in TargetPath. Re-stamp them to the new instance (as in OnPasteNode).
		const FGuid NewGuid = NewRuntimeNode->GetBindingID();
		for (FScriptablePropertyBinding& Binding : NewRuntimeNode->GetPropertyBindings().Bindings)
		{
			Binding.TargetPath.SetStructID(NewGuid);
		}

		if (UScriptableNode_Task* TaskWrapper = Cast<UScriptableNode_Task>(NewRuntimeNode))
		{
			if (UScriptableTask* InnerTask = TaskWrapper->Task)
			{
				const FGuid InnerNewGuid = InnerTask->GetBindingID();
				for (FScriptablePropertyBinding& Binding : InnerTask->GetPropertyBindings().Bindings)
				{
					Binding.TargetPath.SetStructID(InnerNewGuid);
				}
			}
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

void FScriptableGraphEditor::OnCreateComment()
{
	if (!GraphEditorWidget.IsValid()) return;

	UEdGraph* Graph = GraphEditorWidget->GetCurrentGraph();
	if (!Graph) return;

	// If nodes are selected, size the comment to wrap them; otherwise place it at the cursor with a default size.
	FSlateRect BoundsRect;
	const bool bSelectionHasBounds = GraphEditorWidget->GetBoundsForSelectedNodes(BoundsRect, /*Padding*/ 50.0f);

	const FScopedTransaction Transaction(LOCTEXT("CreateCommentTx", "Create Comment"));
	Graph->Modify();

	UEdGraphNode_Comment* CommentNode = NewObject<UEdGraphNode_Comment>(Graph);
	CommentNode->SetFlags(RF_Transactional);
	Graph->AddNode(CommentNode, /*bFromUI*/ true, /*bSelectNewNode*/ true);
	CommentNode->CreateNewGuid();
	CommentNode->PostPlacedNewNode();
	CommentNode->AllocateDefaultPins();

	if (bSelectionHasBounds)
	{
		// Wrap the selection: place top-left at the selection's bounds and size accordingly.
		CommentNode->NodePosX = BoundsRect.Left;
		CommentNode->NodePosY = BoundsRect.Top;
		CommentNode->NodeWidth = BoundsRect.Right - BoundsRect.Left;
		CommentNode->NodeHeight = BoundsRect.Bottom - BoundsRect.Top;
	}
	else
	{
		// Default placement at the paste location with stock dimensions matching BP's convention.
		const FVector2f PasteLocation = GraphEditorWidget->GetPasteLocation2f();
		CommentNode->NodePosX = PasteLocation.X;
		CommentNode->NodePosY = PasteLocation.Y;
		CommentNode->NodeWidth = 400;
		CommentNode->NodeHeight = 100;
	}

	// Select the new comment so the user can start typing the title immediately.
	GraphEditorWidget->ClearSelectionSet();
	GraphEditorWidget->SetNodeSelection(CommentNode, true);

	Graph->NotifyGraphChanged();
}

void FScriptableGraphEditor::OnRemoveSequencePin()
{
	if (!GraphEditorWidget.IsValid()) return;

	UEdGraphPin* Pin = GraphEditorWidget->GetGraphPinForMenu();
	if (!Pin || Pin->Direction != EGPD_Output) return;

	UScriptableEdGraphNode* SfEdNode = Cast<UScriptableEdGraphNode>(Pin->GetOwningNode());
	if (!SfEdNode) return;

	UScriptableNode_Sequence* Sequence = Cast<UScriptableNode_Sequence>(SfEdNode->GetRuntimeNode());
	if (!Sequence) return;

	// Parse the branch index from the pin name (pure numeric per MakeOutputName).
	const FString PinNameStr = Pin->PinName.ToString();
	int32 DigitStart = PinNameStr.Len();
	while (DigitStart > 0 && FChar::IsDigit(PinNameStr[DigitStart - 1]))
	{
		--DigitStart;
	}
	if (DigitStart == PinNameStr.Len()) return;

	const int32 BranchIndex = FCString::Atoi(*PinNameStr.Mid(DigitStart));
	if (BranchIndex < 0 || BranchIndex >= Sequence->OutputCount) return;

	const FScopedTransaction Transaction(LOCTEXT("RemoveSequencePinTx", "Remove Sequence Pin"));
	Sequence->RemoveOutputPinAt(BranchIndex);
	ReconstructEdGraphFromAsset();
}

bool FScriptableGraphEditor::CanRemoveSequencePin() const
{
	if (!GraphEditorWidget.IsValid()) return false;

	UEdGraphPin* Pin = GraphEditorWidget->GetGraphPinForMenu();
	if (!Pin || Pin->Direction != EGPD_Output) return false;

	UScriptableEdGraphNode* SfEdNode = Cast<UScriptableEdGraphNode>(Pin->GetOwningNode());
	if (!SfEdNode) return false;

	const UScriptableNode_Sequence* Sequence = Cast<UScriptableNode_Sequence>(SfEdNode->GetRuntimeNode());
	if (!Sequence) return false;

	return Sequence->OutputCount > UScriptableNode_Sequence::MinOutputCount;
}

void FScriptableGraphEditor::OnRemoveANDPin()
{
	if (!GraphEditorWidget.IsValid()) return;

	UEdGraphPin* Pin = GraphEditorWidget->GetGraphPinForMenu();
	if (!Pin || Pin->Direction != EGPD_Input) return;

	UScriptableEdGraphNode* SfEdNode = Cast<UScriptableEdGraphNode>(Pin->GetOwningNode());
	if (!SfEdNode) return;

	UScriptableNode_AND* AND = Cast<UScriptableNode_AND>(SfEdNode->GetRuntimeNode());
	if (!AND) return;

	// Parse the branch index from the pin name (pure numeric per MakeInputName).
	const FString PinNameStr = Pin->PinName.ToString();
	int32 DigitStart = PinNameStr.Len();
	while (DigitStart > 0 && FChar::IsDigit(PinNameStr[DigitStart - 1]))
	{
		--DigitStart;
	}
	if (DigitStart == PinNameStr.Len()) return;

	const int32 BranchIndex = FCString::Atoi(*PinNameStr.Mid(DigitStart));
	if (BranchIndex < 0 || BranchIndex >= AND->InputCount) return;

	const FScopedTransaction Transaction(LOCTEXT("RemoveANDPinTx", "Remove AND Pin"));
	AND->RemoveInputPinAt(BranchIndex);
	ReconstructEdGraphFromAsset();
}

bool FScriptableGraphEditor::CanRemoveANDPin() const
{
	if (!GraphEditorWidget.IsValid()) return false;

	UEdGraphPin* Pin = GraphEditorWidget->GetGraphPinForMenu();
	if (!Pin || Pin->Direction != EGPD_Input) return false;

	UScriptableEdGraphNode* SfEdNode = Cast<UScriptableEdGraphNode>(Pin->GetOwningNode());
	if (!SfEdNode) return false;

	const UScriptableNode_AND* AND = Cast<UScriptableNode_AND>(SfEdNode->GetRuntimeNode());
	if (!AND) return false;

	return AND->InputCount > UScriptableNode_AND::MinInputCount;
}

void FScriptableGraphEditor::OnObjectTransacted(UObject* Object, const FTransactionObjectEvent& Event)
{
	// UE fires this for every transacted object globally; filter to our asset and to UndoRedo events.
	if (!Object) return;
	if (Object != EditedGraph.Get()) return;
	if (Event.GetEventType() != ETransactionObjectEventType::UndoRedo) return;

	// Rebuild the ed-graph from the asset's now-restored Nodes + Connections, wiping any stale state.
	ReconstructEdGraphFromAsset();
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

	// Single-selection only: anything else (empty or multi) clears the panel.
	if (NewSelection.Num() != 1)
	{
		NodeDetailsView->SetObject(nullptr);
		return;
	}

	UObject* Selected = *NewSelection.CreateConstIterator();

	if (UScriptableEdGraphNode* SfEdNode = Cast<UScriptableEdGraphNode>(Selected))
	{
		UScriptableNode* RuntimeNode = SfEdNode->GetRuntimeNode();
		NodeDetailsView->SetObject(RuntimeNode);
		return;
	}

	if (Selected && Selected->IsA<UEdGraphNode>())
	{
		NodeDetailsView->SetObject(Selected);
		return;
	}

	NodeDetailsView->SetObject(nullptr);
}

TSharedRef<SDockTab> FScriptableGraphEditor::SpawnTab_Validation(const FSpawnTabArgs& Args)
{
	SAssignNew(ValidationPanel, SKzValidationPanel)
		.OnIssueActivated(SKzValidationPanel::FOnIssueActivated::CreateSP(this, &FScriptableGraphEditor::HandleValidationIssueActivated))
		.OnRunValidation(SKzValidationPanel::FOnRunValidation::CreateSP(this, &FScriptableGraphEditor::HandleRunValidation));

	return SNew(SDockTab)
		.Label(LOCTEXT("ValidationTab", "Validation"))
		[
			ValidationPanel.ToSharedRef()
		];
}

void FScriptableGraphEditor::ExtendToolbar()
{
	TSharedPtr<FExtender> Extender = MakeShared<FExtender>();
	Extender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateLambda([this](FToolBarBuilder& ToolbarBuilder)
			{
				ToolbarBuilder.BeginSection("Validation");
				{
					ToolbarBuilder.AddToolBarButton(
						FUIAction(FExecuteAction::CreateSP(this, &FScriptableGraphEditor::OnRunValidation)),
						NAME_None,
						LOCTEXT("ValidateBtn", "Validate"),
						LOCTEXT("ValidateBtnTip", "Run structural validation on this graph and open the Validation tab"),
						FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Refresh"));
				}
				ToolbarBuilder.EndSection();

				ToolbarBuilder.BeginSection("GraphTools");
				{
					ToolbarBuilder.AddToolBarButton(
						FUIAction(FExecuteAction::CreateSP(this, &FScriptableGraphEditor::OnGoHome)),
						NAME_None,
						LOCTEXT("HomeBtn", "Home"),
						LOCTEXT("HomeBtnTip", "Center the view on the Entry node"),
						FSlateIcon(FAppStyle::GetAppStyleSetName(), "MaterialEditor.CameraHome"));

					ToolbarBuilder.AddToolBarButton(
						FUIAction(FExecuteAction::CreateSP(this, &FScriptableGraphEditor::OnCleanGraph)),
						NAME_None,
						LOCTEXT("CleanBtn", "Clean Graph"),
						LOCTEXT("CleanBtnTip", "Delete every node unreachable from Entry or any ReceiveEvent"),
						FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.Clean"));

					ToolbarBuilder.AddToolBarButton(
						FUIAction(FExecuteAction::CreateSP(this, &FScriptableGraphEditor::OnOpenSearch)),
						NAME_None,
						LOCTEXT("SearchBtn", "Search"),
						LOCTEXT("SearchBtnTip", "Open the node search tab"),
						FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Search"));
				}
				ToolbarBuilder.EndSection();
			}));

	AddToolbarExtender(Extender);
}

void FScriptableGraphEditor::OnRunValidation()
{
	if (const TSharedPtr<FTabManager> TabManagerPin = GetTabManager())
	{
		TabManagerPin->TryInvokeTab(ValidationTabId);
	}
	if (ValidationPanel.IsValid())
	{
		ValidationPanel->RefreshIssues();
	}
}

TArray<FKzValidationIssue> FScriptableGraphEditor::HandleRunValidation()
{
	return FKzAssetValidationUtils::RunValidation(EditedGraph.Get());
}

void FScriptableGraphEditor::HandleValidationIssueActivated(const FKzValidationIssue& Issue)
{
	if (!Issue.ContextId.IsValid() || !GraphEditorWidget.IsValid()) return;

	if (UEdGraphNode* EdNode = FindEdNodeByRuntimeId(Issue.ContextId))
	{
		GraphEditorWidget->JumpToNode(EdNode, /*bRequestRename*/ false, /*bSelectNode*/ true);
	}
}

UEdGraphNode* FScriptableGraphEditor::FindEdNodeByRuntimeId(const FGuid& RuntimeId) const
{
	UScriptableGraph* Graph = EditedGraph.Get();
	if (!Graph || !Graph->EdGraph || !RuntimeId.IsValid()) return nullptr;

	for (UEdGraphNode* EdNode : Graph->EdGraph->Nodes)
	{
		const UScriptableEdGraphNode* SfEdNode = Cast<UScriptableEdGraphNode>(EdNode);
		if (SfEdNode && SfEdNode->GetRuntimeNode() && SfEdNode->GetRuntimeNode()->GetBindingID() == RuntimeId)
		{
			return EdNode;
		}
	}
	return nullptr;
}

void FScriptableGraphEditor::OnGoHome()
{
	UScriptableGraph* Graph = EditedGraph.Get();
	if (!Graph || !GraphEditorWidget.IsValid()) return;

	if (UEdGraphNode* EntryEdNode = FindEdNodeByRuntimeId(Graph->EntryNodeID))
	{
		GraphEditorWidget->JumpToNode(EntryEdNode, /*bRequestRename*/ false, /*bSelectNode*/ false);
	}
}

void FScriptableGraphEditor::OnCleanGraph()
{
	UScriptableGraph* Graph = EditedGraph.Get();
	if (!Graph || !Graph->EdGraph || !GraphEditorWidget.IsValid()) return;

	// Reachability from Entry + every ReceiveEvent over forward connections (same rule as the validator).
	TMultiMap<FGuid, FGuid> Adjacency;
	for (const FScriptableGraphConnection& Conn : Graph->Connections)
	{
		Adjacency.Add(Conn.From.NodeID, Conn.To.NodeID);
	}

	TSet<FGuid> Reachable;
	TArray<FGuid> Queue;
	if (Graph->EntryNodeID.IsValid()) { Reachable.Add(Graph->EntryNodeID); Queue.Add(Graph->EntryNodeID); }
	for (const TObjectPtr<UScriptableNode>& Node : Graph->Nodes)
	{
		if (Node && Node->IsA<UScriptableNode_ReceiveEvent>())
		{
			bool bAlready = false;
			Reachable.Add(Node->GetBindingID(), &bAlready);
			if (!bAlready) Queue.Add(Node->GetBindingID());
		}
	}
	for (int32 Head = 0; Head < Queue.Num(); ++Head)
	{
		TArray<FGuid> Targets;
		Adjacency.MultiFind(Queue[Head], Targets);
		for (const FGuid& Target : Targets)
		{
			bool bAlready = false;
			Reachable.Add(Target, &bAlready);
			if (!bAlready) Queue.Add(Target);
		}
	}

	// Gather unreachable, user-deletable ed-nodes (Entry is exempt via CanUserDeleteNode).
	TArray<UEdGraphNode*> ToDelete;
	TSet<FGuid> DeletedIds;
	for (UEdGraphNode* EdNode : Graph->EdGraph->Nodes)
	{
		const UScriptableEdGraphNode* SfEdNode = Cast<UScriptableEdGraphNode>(EdNode);
		if (!SfEdNode || !SfEdNode->GetRuntimeNode()) continue;

		const FGuid Id = SfEdNode->GetRuntimeNode()->GetBindingID();
		if (!Id.IsValid() || Reachable.Contains(Id)) continue;
		if (!EdNode->CanUserDeleteNode()) continue;

		ToDelete.Add(EdNode);
		DeletedIds.Add(Id);
	}

	if (ToDelete.IsEmpty()) return;

	const FScopedTransaction Transaction(LOCTEXT("CleanGraphTx", "Clean Graph"));
	Graph->Modify();
	Graph->EdGraph->Modify();

	// Drop connections touching any node we're about to remove, then destroy the ed-nodes
	// (which also strips their runtime nodes from the asset).
	Graph->Connections.RemoveAll([&DeletedIds](const FScriptableGraphConnection& Conn)
		{
			return DeletedIds.Contains(Conn.From.NodeID) || DeletedIds.Contains(Conn.To.NodeID);
		});

	GraphEditorWidget->ClearSelectionSet();
	for (UEdGraphNode* EdNode : ToDelete)
	{
		EdNode->Modify();
		EdNode->DestroyNode();
	}

	GraphEditorWidget->NotifyGraphChanged();
}

void FScriptableGraphEditor::OnOpenSearch()
{
	if (const TSharedPtr<FTabManager> TabManagerPin = GetTabManager())
	{
		TabManagerPin->TryInvokeTab(SearchTabId);
	}
	if (SearchBox.IsValid())
	{
		FSlateApplication::Get().SetKeyboardFocus(SearchBox);
	}
}

namespace
{
	/** Searchable label: ReceiveEvent shows its EventName, Task wrappers the inner task class, others the node class display name. */
	FString GetNodeSearchLabel(const UScriptableNode* Node)
	{
		if (!Node) return FString();
		if (const UScriptableNode_ReceiveEvent* Receive = Cast<UScriptableNode_ReceiveEvent>(Node))
		{
			return FString::Printf(TEXT("Event: %s"), Receive->EventName.IsNone() ? TEXT("(unnamed)") : *Receive->EventName.ToString());
		}
		if (const UScriptableNode_Task* TaskNode = Cast<UScriptableNode_Task>(Node))
		{
			if (TaskNode->Task) return TaskNode->Task->GetClass()->GetDisplayNameText().ToString();
		}
		return Node->GetClass()->GetDisplayNameText().ToString();
	}
}

TSharedRef<SDockTab> FScriptableGraphEditor::SpawnTab_Search(const FSpawnTabArgs& Args)
{
	TSharedRef<SWidget> Content =
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.f)
		[
			SAssignNew(SearchBox, SSearchBox)
			.HintText(LOCTEXT("SearchHint", "Search nodes by name..."))
			.OnTextChanged(this, &FScriptableGraphEditor::OnSearchTextChanged)
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			SAssignNew(SearchListView, SListView<TWeakObjectPtr<UScriptableNode>>)
			.ListItemsSource(&SearchResults)
			.SelectionMode(ESelectionMode::Single)
			.OnGenerateRow(this, &FScriptableGraphEditor::OnGenerateSearchRow)
			.OnMouseButtonClick(this, &FScriptableGraphEditor::OnSearchResultClicked)
		];

	// Seed with every node so an empty query lists the whole graph.
	OnSearchTextChanged(FText::GetEmpty());

	return SNew(SDockTab)
		.Label(LOCTEXT("SearchTab", "Search"))
		[
			Content
		];
}

void FScriptableGraphEditor::OnSearchTextChanged(const FText& InText)
{
	SearchResults.Reset();

	if (UScriptableGraph* Graph = EditedGraph.Get())
	{
		const FString Query = InText.ToString().TrimStartAndEnd();
		for (const TObjectPtr<UScriptableNode>& Node : Graph->Nodes)
		{
			if (!Node) continue;
			if (Query.IsEmpty() || GetNodeSearchLabel(Node).Contains(Query))
			{
				SearchResults.Add(Node);
			}
		}
	}

	if (SearchListView.IsValid())
	{
		SearchListView->RequestListRefresh();
	}
}

TSharedRef<ITableRow> FScriptableGraphEditor::OnGenerateSearchRow(TWeakObjectPtr<UScriptableNode> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	const FString Label = Item.IsValid() ? GetNodeSearchLabel(Item.Get()) : TEXT("<invalid>");
	return SNew(STableRow<TWeakObjectPtr<UScriptableNode>>, OwnerTable)
		[
			SNew(STextBlock).Text(FText::FromString(Label))
		];
}

void FScriptableGraphEditor::OnSearchResultClicked(TWeakObjectPtr<UScriptableNode> Item)
{
	if (!Item.IsValid() || !GraphEditorWidget.IsValid()) return;

	if (UEdGraphNode* EdNode = FindEdNodeByRuntimeId(Item->GetBindingID()))
	{
		GraphEditorWidget->JumpToNode(EdNode, /*bRequestRename*/ false, /*bSelectNode*/ true);
	}
}

#undef LOCTEXT_NAMESPACE
// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/ScriptableGraphEditor.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraph.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphSchema.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode_Entry.h"
#include "ScriptableFrameworkEd/Graph/ScriptableGraphEditorHelpers.h"
#include "ScriptableFrameworkEd/Customization/Widgets/SScriptableTypePicker.h"
#include "ScriptableFrameworkEditorHelpers.h"
#include "ScriptableNodes/ScriptableGraph.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableNodes/ScriptableNode_Entry.h"
#include "ScriptableTasks/ScriptableTask.h"

#include "GraphEditor.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ScriptableGraphEditor"

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
		.SetDisplayName(LOCTEXT("AssetDetailsTab", "Asset Details"))
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

	SAssignNew(GraphEditorWidget, SGraphEditor)
		.AdditionalCommands(GetToolkitCommands())
		.GraphToEdit(EdGraph)
		.GraphEvents(InEvents);

	return SNew(SDockTab)
		.Label(LOCTEXT("GraphTab", "Graph"))
		[
			GraphEditorWidget.ToSharedRef()
		];
}

TSharedRef<SDockTab> FScriptableGraphEditor::SpawnTab_AssetDetails(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(LOCTEXT("AssetDetailsTab", "Asset Details"))
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

	// Spawn ed-nodes for any runtime node not yet represented.
	for (const TObjectPtr<UScriptableNode>& RuntimeNode : Graph->Nodes)
	{
		if (!RuntimeNode || AlreadyVisualized.Contains(RuntimeNode)) continue;

		UScriptableEdGraphNode* NewEdNode = nullptr;
		if (RuntimeNode->IsA<UScriptableNode_Entry>())
		{
			NewEdNode = NewObject<UScriptableEdGraphNode_Entry>(Graph->EdGraph, UScriptableEdGraphNode_Entry::StaticClass(), NAME_None, RF_Transactional);
		}
		// Other node kinds (Task wrappers, etc.) are handled in later sub-steps.

		if (NewEdNode)
		{
			NewEdNode->SetRuntimeNode(RuntimeNode);
			NewEdNode->CreateNewGuid();
			NewEdNode->AllocateDefaultPins();
			Graph->EdGraph->AddNode(NewEdNode, /*bUserAction*/ false, /*bSelectNewNode*/ false);
		}
	}
}

FActionMenuContent FScriptableGraphEditor::OnCreateNodeMenu(UEdGraph* InGraph, const FVector2f& InNodePosition, const TArray<UEdGraphPin*>& InDraggedPins, bool bAutoExpand, SGraphEditor::FActionMenuClosed InOnMenuClosed)
{
	// Capture spawn context by value so the deferred picker callback can use it after the menu closes.
	const FVector2f CapturedLocation(InNodePosition);
	TArray<UEdGraphPin*> CapturedPins = InDraggedPins;

	TSharedRef<SScriptableTypeSelector> TypeSelector = SNew(SScriptableTypeSelector)
		.TitleText(LOCTEXT("ContextMenuTitle", "Select Node"))
		.BaseClass(UScriptableTask::StaticClass())
		.ClassCategoryMeta(ScriptableFrameworkEditor::MD_TaskCategory)
		.AdditionalBaseClass(UScriptableNode::StaticClass())
		.AdditionalClassCategoryMeta(ScriptableFrameworkEditor::MD_NodeCategory)
		.OnNodeTypePicked(SScriptableTypeSelector::FOnNodeTypePicked::CreateSP(this, &FScriptableGraphEditor::OnNodeMenuTypePicked, InGraph, CapturedLocation, CapturedPins));

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

	if (PickedClass->IsChildOf(UScriptableTask::StaticClass()))
	{
		ScriptableGraphEditorHelpers::SpawnTaskNode(InGraph, const_cast<UClass*>(PickedClass), InLocation, FromPin, /*bSelectNewNode*/ true);
	}
	else if (PickedClass->IsChildOf(UScriptableNode::StaticClass()))
	{
		ScriptableGraphEditorHelpers::SpawnNativeNode(InGraph, const_cast<UClass*>(PickedClass), InLocation, FromPin, /*bSelectNewNode*/ true);
	}

	FSlateApplication::Get().DismissAllMenus();
}

#undef LOCTEXT_NAMESPACE

#undef LOCTEXT_NAMESPACE
// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/ScriptableGraphEditor.h"
#include "ScriptableNodes/ScriptableGraph.h"

#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
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
	// Placeholder until the visual graph is wired up in a later step.
	return SNew(SDockTab)
		.Label(LOCTEXT("GraphTab", "Graph"))
		[
			SNew(STextBlock)
				.Text(LOCTEXT("GraphPlaceholder", "Graph view goes here."))
				.Justification(ETextJustify::Center)
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

#undef LOCTEXT_NAMESPACE
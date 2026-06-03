// Copyright 2026 kirzo

#pragma once

#include "ScriptableFrameworkEditor.h"
#include "ScriptableTypeCache.h"
#include "ScriptableFrameworkEditorStyle.h"

#include "ScriptableTasks/ScriptableTask.h"
#include "ScriptableTasks/ScriptableActionAsset.h"
#include "ScriptableTasks/ScriptableAction.h"

#include "ScriptableConditions/ScriptableCondition.h"
#include "ScriptableConditions/ScriptableRequirementAsset.h"
#include "ScriptableConditions/ScriptableRequirement.h"

#include "ScriptableFrameworkEd/Customization/ScriptableTaskCustomization.h"
#include "ScriptableFrameworkEd/Customization/ScriptableActionCustomization.h"

#include "ScriptableFrameworkEd/Customization/ScriptableRequirementCustomization.h"
#include "ScriptableFrameworkEd/Customization/ScriptableConditionCustomization.h"

#include "ScriptableConditions/ScriptableCondition_NestedRequirement.h"
#include "ScriptableFrameworkEd/Customization/ScriptableConditionNestedRequirementCustomization.h"

#include "ScriptableTasks/ScriptableTask_NestedAction.h"
#include "ScriptableFrameworkEd/Customization/ScriptableTaskNestedActionCustomization.h"

#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableNodes/ScriptableGraph.h"
#include "ScriptableFrameworkEd/Graph/Editor/ScriptableGraphEditor.h"
#include "ScriptableFrameworkEd/Graph/ScriptableGraphPinFactory.h"
#include "ScriptableFrameworkEd/Graph/ScriptableGraphNodeFactory.h"
#include "ScriptableFrameworkEd/Graph/ScriptableGraphCommands.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNodeRegistry.h"

#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/AppStyle.h"
#include "Widgets/SWindow.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/StyleColors.h"

#define LOCTEXT_NAMESPACE "FScriptableFrameworkEditorModule"

void FScriptableFrameworkEditorModule::OnStartupModule()
{
	FScriptableFrameworkEditorStyle::Initialize();
	FScriptableGraphCommands::Register();

	FScriptableEdGraphNodeRegistry::Initialize();

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	ScriptableAssetCategoryBit = AssetTools.RegisterAdvancedAssetCategory("ScriptableFramework", INVTEXT("Scriptable Framework"));

	RegisterAssetTypeAction<UScriptableActionAsset>(ScriptableAssetCategoryBit, INVTEXT("Scriptable Action"), FScriptableFrameworkEditorStyle::ScriptableTaskColor.ToFColor(true));
	RegisterAssetTypeAction<UScriptableRequirementAsset>(ScriptableAssetCategoryBit, INVTEXT("Scriptable Requirement"), FScriptableFrameworkEditorStyle::ScriptableConditionColor.ToFColor(true));
	RegisterAssetTypeAction<UScriptableGraph, FScriptableGraphEditor>(ScriptableAssetCategoryBit, INVTEXT("Scriptable Graph"), FScriptableFrameworkEditorStyle::ScriptableGraphColor.ToFColor(true));

	RegisterPropertyLayout<UScriptableTask, FScriptableTaskCustomization>();
	RegisterPropertyLayout<UScriptableCondition, FScriptableConditionCustomization>();
	RegisterPropertyLayout<FScriptableAction, FScriptableActionCustomization>();
	RegisterPropertyLayout<FScriptableRequirement, FScriptableRequirementCustomization>();
	RegisterPropertyLayout<UScriptableCondition_NestedRequirement, FScriptableConditionNestedRequirementCustomization>();
	RegisterPropertyLayout<UScriptableTask_NestedAction, FScriptableTaskNestedActionCustomization>();

	RegisterPinFactory<FScriptableGraphPinFactory>();
	RegisterNodeFactory<FScriptableGraphNodeFactory>();

	LaunchBlockedHandle = UScriptableGraph::OnLaunchBlockedByCompile.AddRaw(this, &FScriptableFrameworkEditorModule::HandleLaunchBlocked);
	BeginPIEHandle = FEditorDelegates::BeginPIE.AddRaw(this, &FScriptableFrameworkEditorModule::HandleBeginPIE);
	EndPIEHandle = FEditorDelegates::EndPIE.AddRaw(this, &FScriptableFrameworkEditorModule::HandleEndPIE);
}

void FScriptableFrameworkEditorModule::OnShutdownModule()
{
	FEditorDelegates::EndPIE.Remove(EndPIEHandle);
	FEditorDelegates::BeginPIE.Remove(BeginPIEHandle);
	UScriptableGraph::OnLaunchBlockedByCompile.Remove(LaunchBlockedHandle);

	FScriptableEdGraphNodeRegistry::Shutdown();
	FScriptableGraphCommands::Unregister();
	FScriptableFrameworkEditorStyle::Shutdown();
}

void FScriptableFrameworkEditorModule::HandleLaunchBlocked(UScriptableGraph* Asset)
{
	if (!Asset) return;
	CompileBlockedDuringPIE.AddUnique(Asset);
}

void FScriptableFrameworkEditorModule::HandleBeginPIE(const bool bSimulating)
{
	CompileBlockedDuringPIE.Reset();
}

void FScriptableFrameworkEditorModule::HandleEndPIE(const bool bSimulating)
{
	if (CompileBlockedDuringPIE.IsEmpty()) return;

	/** Filter out stale weak pointers; we still want the dialog if at least one survived. */
	TArray<TWeakObjectPtr<UScriptableGraph>> Survivors;
	Survivors.Reserve(CompileBlockedDuringPIE.Num());
	for (const TWeakObjectPtr<UScriptableGraph>& Weak : CompileBlockedDuringPIE)
	{
		if (Weak.IsValid()) Survivors.Add(Weak);
	}
	CompileBlockedDuringPIE.Reset();
	if (Survivors.IsEmpty()) return;

	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("CompileBlockedPIETitle", "Scriptable Graph Compilation Errors"))
		.SizingRule(ESizingRule::Autosized)
		.SupportsMaximize(false)
		.SupportsMinimize(false);

	/**
	 * Click on a link sets this and closes the window; we open the asset after AddModalWindow returns.
	 * Opening from inside the modal loop makes the engine flicker because the new editor window can't
	 * take focus until the modal exits.
	 */
	TSharedRef<TWeakObjectPtr<UScriptableGraph>> PendingOpen = MakeShared<TWeakObjectPtr<UScriptableGraph>>();
	const TWeakPtr<SWindow> WeakWindow = Window;

	TSharedRef<SVerticalBox> LinkList = SNew(SVerticalBox);
	for (const TWeakObjectPtr<UScriptableGraph>& Weak : Survivors)
	{
		TWeakObjectPtr<UScriptableGraph> Captured = Weak;
		LinkList->AddSlot()
			.AutoHeight()
			.Padding(0.f, 2.f)
			.HAlign(HAlign_Left)
			[
				SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
					.ContentPadding(FMargin(0.f))
					.Cursor(EMouseCursor::Hand)
					.OnClicked_Lambda([Captured, PendingOpen, WeakWindow]()
					{
						*PendingOpen = Captured;
						if (TSharedPtr<SWindow> Pin = WeakWindow.Pin())
						{
							Pin->RequestDestroyWindow();
						}
						return FReply::Handled();
					})
					[
						SNew(STextBlock)
							.Text(FText::FromString(Weak.Get()->GetName()))
							.ColorAndOpacity(FStyleColors::AccentBlue)
					]
			];
	}

	const TSharedRef<SWidget> Content =
		SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(16.f)
			[
				SNew(SVerticalBox)

					/** Icon + header text. */
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Top)
							.Padding(0.f, 0.f, 12.f, 0.f)
							[
								SNew(SImage)
									.DesiredSizeOverride(FVector2D(24.f, 24.f))
									.Image(FAppStyle::GetBrush("Icons.WarningWithColor"))
							]
							+ SHorizontalBox::Slot()
							.FillWidth(1.f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
									.AutoWrapText(true)
									.Text(LOCTEXT("CompileBlockedPIEHeader", "The following Scriptable Graphs have unresolved compiler errors."))
							]
					]

					/** Asset links. */
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(36.f, 12.f, 0.f, 0.f)
					[
						LinkList
					]

					/** Close button, bottom-right. */
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Right)
					.Padding(0.f, 16.f, 0.f, 0.f)
					[
						SNew(SButton)
							.Text(LOCTEXT("CompileBlockedPIEClose", "Close"))
							.OnClicked_Lambda([WeakWindow]()
							{
								if (TSharedPtr<SWindow> Pin = WeakWindow.Pin())
								{
									Pin->RequestDestroyWindow();
								}
								return FReply::Handled();
							})
					]
			];

	Window->SetContent(Content);

	const TSharedPtr<SWindow> RootWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
	FSlateApplication::Get().AddModalWindow(Window, RootWindow);

	/** Modal closed; safe to open the asset editor now. */
	if (UScriptableGraph* Graph = PendingOpen->Get())
	{
		if (UAssetEditorSubsystem* Sub = GEditor ? GEditor->GetEditorSubsystem<UAssetEditorSubsystem>() : nullptr)
		{
			Sub->OpenEditorForAsset(Graph);
		}
	}
}

TSharedPtr<FScriptableTypeCache> FScriptableFrameworkEditorModule::GetScriptableTypeCache()
{
	if (!ScriptableTypeCache.IsValid())
	{
		ScriptableTypeCache = MakeShareable(new FScriptableTypeCache());
		ScriptableTypeCache->AddRootClass(UScriptableTask::StaticClass());
		ScriptableTypeCache->AddRootClass(UScriptableCondition::StaticClass());
		ScriptableTypeCache->AddRootClass(UScriptableNode::StaticClass());
	}

	return ScriptableTypeCache;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FScriptableFrameworkEditorModule, ScriptableFrameworkEditor);
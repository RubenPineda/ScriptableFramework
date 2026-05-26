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

#include "ScriptableConditions/ScriptableCondition_Group.h"
#include "ScriptableFrameworkEd/Customization/ScriptableConditionGroupCustomization.h"

#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableNodes/ScriptableGraph.h"
#include "ScriptableFrameworkEd/Graph/ScriptableGraphEditor.h"
#include "ScriptableFrameworkEd/Graph/ScriptableGraphPinFactory.h"
#include "ScriptableFrameworkEd/Graph/ScriptableGraphNodeFactory.h"
#include "ScriptableFrameworkEd/Graph/ScriptableGraphCommands.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNodeRegistry.h"

#define LOCTEXT_NAMESPACE "FScriptableFrameworkEditorModule"

void FScriptableFrameworkEditorModule::OnStartupModule()
{
	FScriptableFrameworkEditorStyle::Initialize();
	FScriptableGraphCommands::Register();

	FScriptableEdGraphNodeRegistry::Build();

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	ScriptableAssetCategoryBit = AssetTools.RegisterAdvancedAssetCategory("ScriptableFramework", INVTEXT("Scriptable Framework"));

	RegisterAssetTypeAction<UScriptableActionAsset>(ScriptableAssetCategoryBit, INVTEXT("Scriptable Action"), FScriptableFrameworkEditorStyle::ScriptableTaskColor.ToFColor(true));
	RegisterAssetTypeAction<UScriptableRequirementAsset>(ScriptableAssetCategoryBit, INVTEXT("Scriptable Requirement"), FScriptableFrameworkEditorStyle::ScriptableConditionColor.ToFColor(true));
	RegisterAssetTypeAction<UScriptableGraph, FScriptableGraphEditor>(ScriptableAssetCategoryBit, INVTEXT("Scriptable Graph"), FScriptableFrameworkEditorStyle::ScriptableGraphColor.ToFColor(true));

	RegisterPropertyLayout<UScriptableTask, FScriptableTaskCustomization>();
	RegisterPropertyLayout<UScriptableCondition, FScriptableConditionCustomization>();
	RegisterPropertyLayout<FScriptableAction, FScriptableActionCustomization>();
	RegisterPropertyLayout<FScriptableRequirement, FScriptableRequirementCustomization>();
	RegisterPropertyLayout<UScriptableCondition_Group, FScriptableConditionGroupCustomization>();

	RegisterPinFactory<FScriptableGraphPinFactory>();
	RegisterNodeFactory<FScriptableGraphNodeFactory>();
}

void FScriptableFrameworkEditorModule::OnShutdownModule()
{
	FScriptableGraphCommands::Unregister();
	FScriptableFrameworkEditorStyle::Shutdown();
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
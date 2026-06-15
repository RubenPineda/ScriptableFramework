// Copyright 2026 kirzo

using UnrealBuildTool;

public class ScriptableFrameworkEditor : ModuleRules
{
	public ScriptableFrameworkEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"ScriptableFramework",
				"PropertyAccessEditor",
				"PropertyBindingUtils",
				"KzLibEditor"
			});

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"PropertyEditor",
				"ClassViewer",
				"InputCore",
				"Projects",
				"BlueprintGraph",
				"KismetWidgets",
				"ApplicationCore",
				"DataValidation",
				"DeveloperSettings",
				"ToolMenus",
				"GraphEditor",
				"Kismet",
				"AssetDefinition",
				"SourceControl",
				"KzLib",
				"KzLibUncooked",
				"StructUtilsEditor",
				"WorkspaceMenuStructure"
			});
	}
}
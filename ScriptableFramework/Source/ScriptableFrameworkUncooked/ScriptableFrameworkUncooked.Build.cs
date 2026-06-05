// Copyright 2026 kirzo

using UnrealBuildTool;

public class ScriptableFrameworkUncooked : ModuleRules
{
	public ScriptableFrameworkUncooked(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"ScriptableFramework"
			});

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"BlueprintGraph",
				"KismetCompiler",
				"KzLib"
			});
	}
}
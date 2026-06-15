// Copyright 2026 kirzo

using UnrealBuildTool;

public class ScriptableFrameworkConverters : ModuleRules
{
	public ScriptableFrameworkConverters(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"ScriptableFramework",
				"KzLib"
			});

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"CoreUObject",
				"Engine"
			});
	}
}
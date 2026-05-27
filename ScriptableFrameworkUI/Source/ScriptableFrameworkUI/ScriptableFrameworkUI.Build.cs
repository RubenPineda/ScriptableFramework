// Copyright 2026 kirzo

using UnrealBuildTool;

public class ScriptableFrameworkUI : ModuleRules
{
	public ScriptableFrameworkUI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"UMG"
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"ScriptableFramework"
			}
			);
	}
}

// Copyright 2026 kirzo

using UnrealBuildTool;

public class ScriptableFrameworkSequencer : ModuleRules
{
	public ScriptableFrameworkSequencer(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"LevelSequence",
				"MovieScene"
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
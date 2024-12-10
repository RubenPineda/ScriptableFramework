// Copyright Kirzo. All Rights Reserved.

using UnrealBuildTool;

public class ScriptableFrameworkEditor : ModuleRules
{
    public ScriptableFrameworkEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "ScriptableFramework",
            }
            );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "UnrealEd",
                "PropertyEditor",
                "InputCore",
                "BlueprintGraph",
                "KismetWidgets",
            }
            );
    }
}
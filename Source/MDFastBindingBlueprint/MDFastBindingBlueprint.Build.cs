using UnrealBuildTool;

public class MDFastBindingBlueprint : ModuleRules
{
    public MDFastBindingBlueprint(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "Kismet",
                "UMGEditor"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
	            "ApplicationCore",
	            "BlueprintGraph",
                "CoreUObject",
                "Engine",
                "GraphEditor",
                "MDFastBinding",
                "Slate",
                "SlateCore",
                "UMG",
                "UnrealEd"
            }
        );
    }
}
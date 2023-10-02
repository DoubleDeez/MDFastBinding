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
                "MDFastBinding",
                "UMGEditor",
            }
        );

#if UE_5_3_OR_LATER
        PublicDependencyModuleNames.Add("FieldNotification");
#endif

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
	            "ApplicationCore",
	            "BlueprintGraph",
                "CoreUObject",
                "Engine",
                "GraphEditor",
                "Slate",
                "SlateCore",
                "UMG",
                "UnrealEd"
            }
        );
    }
}
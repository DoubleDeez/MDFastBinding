using UnrealBuildTool;

public class MDFastBinding : ModuleRules
{
	public MDFastBinding(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"UMG"
			}
		);

#if UE_5_3_OR_LATER
		PublicDependencyModuleNames.Add("FieldNotification");
#endif

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore"
			}
		);
	}
}

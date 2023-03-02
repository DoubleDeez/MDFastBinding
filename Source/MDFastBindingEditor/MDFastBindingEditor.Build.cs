// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MDFastBindingEditor : ModuleRules
{
	public MDFastBindingEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		OptimizeCode = CodeOptimization.Never;
		
		PublicIncludePaths.AddRange(
			new string[] {
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"DeveloperSettings",
				"GraphEditor",
				"MDFastBinding", 
				"PropertyEditor",
				"UMGEditor"
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"ApplicationCore",
				"BlueprintGraph",
				"CoreUObject",
				"EditorStyle",
				"Engine",
				"InputCore",
				"Kismet",
				"Projects",
				"Slate",
				"SlateCore",
				"UMG",
				"UnrealEd"
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
			);
	}
}

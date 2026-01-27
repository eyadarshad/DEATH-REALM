// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class MazeRunner : ModuleRules
{
	public MazeRunner(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore",
			"UMG",           // For UI widgets
			"AIModule",      // For AI pathfinding
			"Niagara"        // For weather particle effects
		});

		PrivateDependencyModuleNames.AddRange(new string[] { 
			"Slate",         // UI framework
			"SlateCore"      // UI core
		});
	}
}

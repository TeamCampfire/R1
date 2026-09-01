// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class R1 : ModuleRules
{
	public R1(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AnimGraphRuntime",
			"UMG",
			"Slate",
			"SlateCore",
			"Water",
			"CableComponent",
			"Niagara",
			"OnlineSubsystem",		// 세션 관리
			"OnlineSubsystemUtils"	// 세션 관리
		});


		PrivateDependencyModuleNames.AddRange(new string[] {  });

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "UMGEditor" });
		}
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}

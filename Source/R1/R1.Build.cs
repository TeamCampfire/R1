// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class R1 : ModuleRules
{
	public R1(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "AnimGraphRuntime", "UMG", "Slate", "SlateCore", "Water", "CableComponent", "Niagara" });


		PrivateDependencyModuleNames.AddRange(new string[] { "GameplayTags" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}

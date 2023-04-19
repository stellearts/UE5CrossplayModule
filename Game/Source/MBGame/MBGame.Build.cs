// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;


public class MBGame : ModuleRules
{
	public MBGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			//
			// Online features
			"OnlineSubsystem",
			"OnlineSubsystemUtils",
			"OnlineSubsystemEOS",
			"OnlineSubsystemSteam",
			// ASP
			"AdvancedSessions",
			"AdvancedSteamSessions",
			//
			// Voice chat
			"VoiceChat",
			//
			// UMG, Slate and SlateCore are for the UserWidget class.
			"UMG",
			"Slate",
			"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			
		});
	}
}

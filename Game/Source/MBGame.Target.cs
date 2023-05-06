// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MBGameTarget : TargetRules
{
	public MBGameTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_1;
		ExtraModuleNames.Add("MBGame");
		RegisterModulesCreatedByRider();
	}

	private void RegisterModulesCreatedByRider()
	{
		ExtraModuleNames.AddRange(new string[] { "OnlineMultiplayer" });
	}
}

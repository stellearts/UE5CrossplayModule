// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MBGameEditorTarget : TargetRules
{
	public MBGameEditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
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

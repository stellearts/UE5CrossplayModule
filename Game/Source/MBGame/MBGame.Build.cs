// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;


public class MBGame : ModuleRules
{
	public MBGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(new string[] {
			// Main dependencies
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			//
			// Online features
			"VoiceChat",
			//
			// UserWidget c++ dependencies.
			"UMG",
			"Slate",
			"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[]{});

		// EOS
		var EOSSDKPath = Path.Combine(ModuleDirectory, "..", "..", "ThirdParty", "EOSSDK");
		PublicIncludePaths.Add(Path.Combine(EOSSDKPath, "Include"));
		PublicAdditionalLibraries.Add(Path.Combine(EOSSDKPath, "Lib", "EOSSDK-Win64-Shipping.lib"));
		
		
		// Steamworks-SDK
		// var SteamworksSDKPath = Path.Combine(ModuleDirectory, "..", "..", "ThirdParty", "SteamworksSDK");
		// PublicIncludePaths.Add(Path.Combine(SteamworksSDKPath, "public", "steam"));
		// PublicAdditionalLibraries.Add(Path.Combine(SteamworksSDKPath, "redistributable_bin", "win64", "steam_api64.lib"));
		
		// Steamworks
		if (Target.Platform == UnrealTargetPlatform.Win64)
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "Public", "Steam", "lib", "steam_api64.lib"));
		else if (Target.Platform == UnrealTargetPlatform.Linux)
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "Public", "Steam", "lib", "libsteam_api.so"));
		
		try
		{
			var SteamDLLFileName = "steam_api64.dll";
			var SteamDLLPath = Path.Combine(ModuleDirectory, "Steam", "lib", SteamDLLFileName);
			var ProjectPath = Directory.GetParent(ModuleDirectory).Parent.ToString();
			var BinariesDir = Path.Combine(ProjectPath, "Binaries", Target.Platform.ToString());

			if (!Directory.Exists(BinariesDir))
				Directory.CreateDirectory(BinariesDir);

			File.Copy(SteamDLLPath, Path.Combine(BinariesDir, SteamDLLFileName), true);
		} catch { }
		
	}
}

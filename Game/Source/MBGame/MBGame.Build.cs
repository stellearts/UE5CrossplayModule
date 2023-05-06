// Copyright © 2023 Melvin Brink

using UnrealBuildTool;
using System.IO;


public class MBGame : ModuleRules
{
	public MBGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(new string[] {
			"OnlineMultiplayer",
			//
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

	}
}

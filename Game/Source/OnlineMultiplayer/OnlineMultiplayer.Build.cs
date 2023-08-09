// Copyright © 2023 Melvin Brink

using UnrealBuildTool;
using System.IO;

public class OnlineMultiplayer : ModuleRules
{
    public OnlineMultiplayer(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
                {
                    "Engine",
                    "Core",
                    "CoreOnline",
                    "CoreUObject",
                    "UMG",
                    "Slate",
                    "SlateCore"
                }
        );

        PrivateDependencyModuleNames.AddRange(new string[]{});
        
        
        
        
        /*
         * Linking libraries, DLL's and copying DLL's to binaries directory for the SDKs.
        */
        
        // Get project path and create binaries directory if it doesn't exist.
        var projectPath = Directory.GetParent(ModuleDirectory).Parent.ToString();
        var binariesDir = Path.Combine(projectPath, "Binaries", Target.Platform.ToString());
        if (!Directory.Exists(binariesDir))
            Directory.CreateDirectory(binariesDir);
        
        // Stage DLL's next to executable directory.
        RuntimeDependencies.Add("$(TargetOutputDir)/EOSSDK-Win64-Shipping.dll", "$(ProjectDir)/Source/OnlineMultiplayer/SDK/EOS/Bin/EOSSDK-Win64-Shipping.dll");
        RuntimeDependencies.Add("$(TargetOutputDir)/steam_api64.dll", "$(ProjectDir)/Source/OnlineMultiplayer/SDK/Steamworks/lib/steam_api64.dll");
        
        // Stage steam_appid.txt in a build other than shipping.
        if (Target.Configuration != UnrealTargetConfiguration.Shipping)
        {
            RuntimeDependencies.Add("$(TargetOutputDir)/steam_appid.txt", "$(ProjectDir)/Source/OnlineMultiplayer/SDK/Steamworks/lib/steam_appid.txt");
        }
        

        
        //EOS SDK
        var eosSdkPath = Path.Combine(ModuleDirectory, "SDK", "EOS");
        PublicIncludePaths.Add(Path.Combine(eosSdkPath, "Include"));
        PublicAdditionalLibraries.Add(Path.Combine(eosSdkPath, "Lib", "EOSSDK-Win64-Shipping.lib"));
        
        try
        {
            const string eosDllFileName = "EOSSDK-Win64-Shipping.dll";
            var eosDllPath = Path.Combine(eosSdkPath, "Bin", eosDllFileName);
            File.Copy(eosDllPath, Path.Combine(binariesDir, eosDllFileName), true);
        } catch { }
        
        
        // Steamworks SDK
        var steamworksSdkPath = Path.Combine(ModuleDirectory, "SDK", "Steamworks");
        PublicIncludePaths.Add(steamworksSdkPath);
            
        if (Target.Platform == UnrealTargetPlatform.Win64)
            PublicAdditionalLibraries.Add(Path.Combine(steamworksSdkPath, "lib", "steam_api64.lib"));
        else if (Target.Platform == UnrealTargetPlatform.Linux)
            PublicAdditionalLibraries.Add(Path.Combine(steamworksSdkPath, "lib", "libsteam_api.so"));
		
        try
        {
            const string steamDllFileName = "steam_api64.dll";
            var steamDllPath = Path.Combine(steamworksSdkPath, "lib", steamDllFileName);
            File.Copy(steamDllPath, Path.Combine(binariesDir, steamDllFileName), true);
        } catch { }
        
    }
}
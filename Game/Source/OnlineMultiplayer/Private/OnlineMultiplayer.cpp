// Copyright © 2023 Melvin Brink

#include "OnlineMultiplayer.h"
#include "Modules/ModuleManager.h"
#include "Platforms/Steam/SteamManager.h"
#include "Platforms/EOS/EOSManager.h"



/**
 * Startup for initializing the Steamworks/EOS SDK's.
*/
void FOnlineMultiplayer::StartupModule()
{
	UE_LOG(LogTemp, Warning, TEXT("FOnlineMultiplayer::StartupModule"));

	
	// TODO: Initialize the correct SDK for the platform the user is on.
	// TODO: Check if singleton like this is best way? And rename the managers from PlatformNameManager to PlatformNameSDK?
	
	// Initialize the Steamworks SDK.
	FSteamManager::Get().Initialize();

	// Initialize the EOS SDK.
	FEosManager::Get().Initialize();
}

void FOnlineMultiplayer::ShutdownModule()
{
	// TODO: DeInitialize the correct SDK for the platform the user is on.
	FSteamManager::Get().DeInitialize();
}

IMPLEMENT_MODULE(FOnlineMultiplayer, OnlineMultiplayer)
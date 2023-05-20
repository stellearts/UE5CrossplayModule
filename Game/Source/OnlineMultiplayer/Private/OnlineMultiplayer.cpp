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
	
	// Initialize the Steamworks SDK.
	FSteamManager::Get().Initialize();

	// Initialize the EOS SDK.
	FEosManager::Get().Initialize();
}

void FOnlineMultiplayer::ShutdownModule()
{
	FSteamManager::Get().DeInitialize();
}

IMPLEMENT_MODULE(FOnlineMultiplayer, OnlineMultiplayer)
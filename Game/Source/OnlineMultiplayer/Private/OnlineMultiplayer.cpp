// Copyright © 2023 Melvin Brink

#include "OnlineMultiplayer.h"
#include "Modules/ModuleManager.h"
#include "SteamManager.h"
#include "EOSManager.h"



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

/**
 * Keeps checking if the subsystems in this module are fully done initializing.
 *
 * Use this whenever you want to make sure every variable on any subsystem is ready before using them.
 * For instance, this function will wait when a user has fetched all information needed for a lobby / session, because we might be in a lobby when starting the game, and we need that data.
 *
 * This makes it easy to use in other modules since you don't have to add the necessary logic yourself.
 *
 * @param Callback the callback to run after the subsystems are initialized.
 */
void FOnlineMultiplayer::WaitUntilReady(TFunction<void()>& Callback)
{
}

IMPLEMENT_MODULE(FOnlineMultiplayer, OnlineMultiplayer)

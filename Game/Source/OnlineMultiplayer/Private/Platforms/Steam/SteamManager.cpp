// Copyright © 2023 Melvin Brink

#include "Platforms/Steam/SteamManager.h"

#pragma warning(push)
#pragma warning(disable: 4996)
#pragma warning(disable: 4265)
#include "steam_api.h"
#include "steamnetworkingtypes.h"
#pragma warning(pop)



void SteamAPIDebugMessageHook(int nSeverity, const char *pchDebugText)
{
	// Output the debug message to your desired logging method
	printf("SteamAPI Debug: %s\n", pchDebugText);
}

FSteamManager::FSteamManager()
{
	
}

void FSteamManager::Tick(float DeltaTime)
{
	SteamAPI_RunCallbacks();
}

bool FSteamManager::IsTickable() const
{
	return true;
}

TStatId FSteamManager::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FSteamManager, STATGROUP_Tickables);
}

FSteamManager& FSteamManager::Get()
{
	static FSteamManager Instance;
	return Instance;
}


// --------------------------------


void FSteamManager::Initialize()
{
	UE_LOG(LogSteamManager, Log, TEXT("Initializing Steam-Subsystem"));
	
	if (SteamAPI_RestartAppIfNecessary(2428930))
	{
		// The app is being restarted to use the specified AppID; exit gracefully
		FPlatformMisc::RequestExit(false);
		return;
	}
	
	if (SteamAPI_Init())
	{
		UE_LOG(LogSteamManager, Log, TEXT("Steamworks SDK Initialized"));
		SteamUtils()->SetWarningMessageHook(SteamAPIDebugMessageHook);
	}
	else
	{
		UE_LOG(LogSteamManager, Error, TEXT("Failed to initialize Steamworks SDK"));
	}
}

void FSteamManager::DeInitialize()
{
	if (SteamAPI_Init()) SteamAPI_Shutdown();
}
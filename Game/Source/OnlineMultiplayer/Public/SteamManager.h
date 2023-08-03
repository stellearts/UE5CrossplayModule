// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include <string>
#pragma warning(push)
#pragma warning(disable: 4996)
#pragma warning(disable: 4265)
#include "steam_api.h"
#include "steam_api_common.h"
#include "isteamuser.h"
#pragma warning(pop)

DECLARE_LOG_CATEGORY_EXTERN(LogSteamManager, Log, All);
inline DEFINE_LOG_CATEGORY(LogSteamManager);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnEncryptedAppTicketReady, TArray<uint8>);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSessionTicketReady, TArray<uint8>);



/**
 * Singleton class for the Steam-SDK.
 * Responsible for initializing the SDK and providing helper functions for other classes.
 */
class ONLINEMULTIPLAYER_API FSteamManager : public FTickableGameObject
{
	// Private constructor to prevent direct instantiation
	FSteamManager();
	
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

public:
	static FSteamManager& Get();
	void Initialize();
	void DeInitialize();
};

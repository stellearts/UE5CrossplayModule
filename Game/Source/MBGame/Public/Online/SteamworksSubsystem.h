// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "Steam/steam_api.h"
#include "Steam/steam_api_common.h"
#include "Steam/isteamuser.h"
#include "SteamworksSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSteamSubsystem, Log, All);
inline DEFINE_LOG_CATEGORY(LogSteamSubsystem);

#define LOG_STEAM_NULL UE_LOG(LogSteamSubsystem, Error, TEXT("Steam API is not initialized."));

DECLARE_MULTICAST_DELEGATE(FOnEncryptedAppTicketReady);
DECLARE_MULTICAST_DELEGATE(FOnAuthSessionTicketReady);



/**
 * 
 */
UCLASS()
class MBGAME_API USteamworksSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
    void InitializeSteamworks();
    CCallResult<USteamworksSubsystem, EncryptedAppTicketResponse_t> m_EncryptedAppTicketResponse;

public:
	// Encrypted App Ticket
	void RequestSteamEncryptedAppTicket();
	TArray<uint8> GetEncryptedAppTicket();
	FOnEncryptedAppTicketReady OnEncryptedAppTicketReady;
    void OnEncryptedAppTicketResponse(EncryptedAppTicketResponse_t* pEncryptedAppTicketResponse, bool bIOFailure);

	// Auth Session Ticket
	TArray<uint8> GetAuthSessionTicket();

private:
	SteamNetworkingIdentity Identity;
};

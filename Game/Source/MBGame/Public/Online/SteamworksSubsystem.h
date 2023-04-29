// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"

#pragma warning(push)
#pragma warning(disable: 4996)
#include "Steam/steam_api.h"
#include "Steam/steam_api_common.h"
#include "Steam/isteamuser.h"
#pragma warning(pop)

#include "SteamworksSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSteamSubsystem, Log, All);
inline DEFINE_LOG_CATEGORY(LogSteamSubsystem);

#define LOG_STEAM_NULL UE_LOG(LogSteamSubsystem, Error, TEXT("Steam API is not initialized."));

DECLARE_MULTICAST_DELEGATE(FOnEncryptedAppTicketReady);
DECLARE_MULTICAST_DELEGATE(FOnSessionTicketReady);



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
	TArray<uint8> SessionTicket;
	FORCEINLINE TArray<uint8> GetSessionTicket() { return SessionTicket; }
	TArray<uint8> RequestSessionTicketSteam();
	FOnSessionTicketReady OnSessionTicketReady;
	STEAM_CALLBACK(USteamworksSubsystem, OnSessionTicketResponse, GetAuthSessionTicketResponse_t);
	
private:
	SteamNetworkingIdentity Identity;
};

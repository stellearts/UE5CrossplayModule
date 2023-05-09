// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"

#pragma warning(push)
#pragma warning(disable: 4996)
#pragma warning(disable: 4265)
#include "steam_api.h"
#include "steam_api_common.h"
#include "isteamuser.h"
#pragma warning(pop)

DECLARE_LOG_CATEGORY_EXTERN(LogSteamManager, Log, All);
inline DEFINE_LOG_CATEGORY(LogSteamManager);

#define LOG_STEAM_NULL UE_LOG(LogSteamManager, Error, TEXT("Steam SDK is not initialized."));

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
	// Prevent copy-construction
	FSteamManager(const FSteamManager&) = delete;
	// Prevent assignment
	FSteamManager& operator=(const FSteamManager&) = delete;
	
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

public:
	static FSteamManager& Get();
	void Initialize();
	void DeInitialize();

	
	// Tickets
	void RequestEncryptedAppTicket();
	FOnEncryptedAppTicketReady OnEncryptedAppTicketReady;
	void RequestSessionTicket();
	FOnSessionTicketReady OnSessionTicketReady;
	
private:
	// Ticket callbacks
	void OnEncryptedAppTicketResponse( EncryptedAppTicketResponse_t *pEncryptedAppTicketResponse, bool bIOFailure );
	CCallResult<FSteamManager, EncryptedAppTicketResponse_t> m_EncryptedAppTicketResponseCallResult;
	STEAM_CALLBACK(FSteamManager, OnSessionTicketResponse, GetAuthSessionTicketResponse_t);

	// Session ticket data
	TArray<uint8> SessionTicket;
	bool bWaitingForSessionTicket = false;
	bool bSessionTicketReady = false;
	
	// Game callbacks
	STEAM_CALLBACK(FSteamManager, JoinLobbyRequest, GameLobbyJoinRequested_t);
	STEAM_CALLBACK(FSteamManager, JoinRichPresenceRequest, GameRichPresenceJoinRequested_t);
	
	SteamNetworkingIdentity Identity;

public:
	FORCEINLINE CSteamID GetSteamID() const { return SteamUser()->GetSteamID(); }
};

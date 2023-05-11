// Copyright © 2023 Melvin Brink

#pragma once

#pragma warning(push)
#pragma warning(disable: 4996)
#pragma warning(disable: 4265)
#include "steam_api.h"
#include "steam_api_common.h"
#include "isteamuser.h"
#include "steamnetworkingtypes.h"
#pragma warning(pop)

DECLARE_LOG_CATEGORY_EXTERN(LogSteamLobbyManager, Log, All);
inline DEFINE_LOG_CATEGORY(LogSteamLobbyManager);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnCreateShadowLobbyCompleteDelegate, uint64)
DECLARE_MULTICAST_DELEGATE_OneParam(FOnUserJoinShadowLobbyDelegate, CSteamID)
DECLARE_MULTICAST_DELEGATE_OneParam(FOnUserLeaveShadowLobbyDelegate, CSteamID)



class FSteamLobbyManager
{

public:
	FSteamLobbyManager() = default;
	~FSteamLobbyManager() = default;
	
	void CreateShadowLobby(const char* EosLobbyID);
	FOnCreateShadowLobbyCompleteDelegate OnCreateShadowLobbyCompleteDelegate;
	
	FOnUserJoinShadowLobbyDelegate OnUserJoinSteamLobbyDelegate;
	FOnUserLeaveShadowLobbyDelegate OnUserLeaveSteamLobbyDelegate;

private:
	void OnCreateShadowLobbyComplete(LobbyCreated_t* Data, bool bIOFailure);
	CCallResult<FSteamLobbyManager, LobbyCreated_t> OnCreateShadowLobbyCallResult;

	void OnShadowLobbyEntered(LobbyEnter_t* Data, bool bIOFailure);
	CCallResult<FSteamLobbyManager, LobbyEnter_t> OnShadowLobbyEnterCallResult;
	
	STEAM_CALLBACK(FSteamLobbyManager, OnJoinShadowLobbyRequest, GameLobbyJoinRequested_t);
	STEAM_CALLBACK(FSteamLobbyManager, OnJoinRichPresenceRequest, GameRichPresenceJoinRequested_t);
};

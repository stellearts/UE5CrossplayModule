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
DECLARE_MULTICAST_DELEGATE_OneParam(FOnJoinShadowLobbyCompleteDelegate, uint64)
DECLARE_MULTICAST_DELEGATE_OneParam(FOnUserJoinShadowLobbyDelegate, CSteamID)
DECLARE_MULTICAST_DELEGATE_OneParam(FOnUserLeaveShadowLobbyDelegate, CSteamID)



class FSteamLobbyManager
{

public:
	explicit FSteamLobbyManager(UGameInstance* InGameInstance);
	~FSteamLobbyManager() = default;
	
	void CreateShadowLobby();
	FOnCreateShadowLobbyCompleteDelegate OnCreateShadowLobbyCompleteDelegate;
	
	void JoinShadowLobby(uint64 SteamLobbyID);
	FOnJoinShadowLobbyCompleteDelegate OnJoinShadowLobbyCompleteDelegate;
	
	FOnUserJoinShadowLobbyDelegate OnUserJoinSteamLobbyDelegate;
	FOnUserLeaveShadowLobbyDelegate OnUserLeaveSteamLobbyDelegate;

private:
	void OnCreateShadowLobbyComplete(LobbyCreated_t* Data, bool bIOFailure);
	CCallResult<FSteamLobbyManager, LobbyCreated_t> OnCreateShadowLobbyCallResult;

	void OnJoinShadowLobbyComplete(LobbyEnter_t* Data, bool bIOFailure);
	CCallResult<FSteamLobbyManager, LobbyEnter_t> OnShadowLobbyEnterCallResult;

	STEAM_CALLBACK(FSteamLobbyManager, OnLobbyDataUpdateComplete, LobbyDataUpdate_t);
	STEAM_CALLBACK(FSteamLobbyManager, OnJoinShadowLobbyRequest, GameLobbyJoinRequested_t);
	STEAM_CALLBACK(FSteamLobbyManager, OnJoinRichPresenceRequest, GameRichPresenceJoinRequested_t);

	UGameInstance* GameInstance;
};

// Copyright © 2023 Melvin Brink

#pragma once

#pragma warning(push)
#pragma warning(disable: 4996)
#pragma warning(disable: 4265)
#include "steam_api.h"
#include "steam_api_common.h"
#include "steamnetworkingtypes.h"
#pragma warning(pop)

#include "CoreMinimal.h"
#include "SteamLobbySubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSteamLobbySubsystem, Log, All);
inline DEFINE_LOG_CATEGORY(LogSteamLobbySubsystem);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnCreateShadowLobbyCompleteDelegate, uint64);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnJoinShadowLobbyCompleteDelegate, uint64);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnUserJoinShadowLobbyDelegate, CSteamID);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnUserLeaveShadowLobbyDelegate, CSteamID);



/**
 * Subsystem for managing Steam friends.
 */
UCLASS(NotBlueprintable)
class ONLINEMULTIPLAYER_API USteamLobbySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

public:
	void CreateShadowLobby();
	FOnCreateShadowLobbyCompleteDelegate OnCreateShadowLobbyCompleteDelegate;
	
	void JoinShadowLobby(uint64 SteamLobbyID);
	FOnJoinShadowLobbyCompleteDelegate OnJoinShadowLobbyCompleteDelegate;
	
	FOnUserJoinShadowLobbyDelegate OnUserJoinSteamLobbyDelegate;
	FOnUserLeaveShadowLobbyDelegate OnUserLeaveSteamLobbyDelegate;

private:
	void OnCreateShadowLobbyComplete(LobbyCreated_t* Data, bool bIOFailure);
	CCallResult<USteamLobbySubsystem, LobbyCreated_t> OnCreateShadowLobbyCallResult;

	void OnJoinShadowLobbyComplete(LobbyEnter_t* Data, bool bIOFailure);
	CCallResult<USteamLobbySubsystem, LobbyEnter_t> OnShadowLobbyEnterCallResult;

	STEAM_CALLBACK(USteamLobbySubsystem, OnLobbyDataUpdateComplete, LobbyDataUpdate_t);
	STEAM_CALLBACK(USteamLobbySubsystem, OnJoinShadowLobbyRequest, GameLobbyJoinRequested_t);
	STEAM_CALLBACK(USteamLobbySubsystem, OnJoinRichPresenceRequest, GameRichPresenceJoinRequested_t);
};

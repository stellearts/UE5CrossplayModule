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
#include "PlatformLobbySubsystemBase.h"
#include "SteamLobbySubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSteamLobbySubsystem, Log, All);
inline DEFINE_LOG_CATEGORY(LogSteamLobbySubsystem);



/**
 * Subsystem for managing Steam friends
 */
UCLASS()
class ONLINEMULTIPLAYER_API USteamLobbySubsystem : public UPlatformLobbySubsystemBase
{
	GENERATED_BODY()

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

public:
	virtual void CreateLobby() override;
	virtual void JoinLobby(const FString& LobbyID) override;
	virtual void LeaveLobby() override;
	

private:
	void OnCreateLobbyComplete(LobbyCreated_t* Data, bool bIOFailure);
	CCallResult<USteamLobbySubsystem, LobbyCreated_t> OnCreateShadowLobbyCallResult;

	void OnJoinLobbyComplete(LobbyEnter_t* Data, bool bIOFailure);
	CCallResult<USteamLobbySubsystem, LobbyEnter_t> OnShadowLobbyEnterCallResult;

	STEAM_CALLBACK(USteamLobbySubsystem, OnLobbyDataUpdateComplete, LobbyDataUpdate_t);
	STEAM_CALLBACK(USteamLobbySubsystem, OnJoinLobbyRequest, GameLobbyJoinRequested_t);
	STEAM_CALLBACK(USteamLobbySubsystem, OnJoinRichPresenceRequest, GameRichPresenceJoinRequested_t);

	UPROPERTY() class ULocalUserSubsystem* LocalUserSubsystem;
	UPROPERTY() class ULobbySubsystem* LobbySubsystem;
	
	FShadowLobbyDetails LobbyDetails;

public:
	FORCEINLINE const FShadowLobbyDetails& GetLobbyDetails() const { return LobbyDetails; }
	FORCEINLINE bool InLobby() const { return !LobbyDetails.LobbyID.IsEmpty(); }
};

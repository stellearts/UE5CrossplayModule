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
UCLASS(NotBlueprintable)
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
	FORCEINLINE bool InLobby() const { return !LobbyDetails.LobbyID.IsEmpty(); }

	// Lobby-Details Getters
	FORCEINLINE FString GetLobbyID() const { return LobbyDetails.LobbyID; }
	FORCEINLINE FString GetLobbyOwnerID() const { return LobbyDetails.LobbyOwnerID; }
	FORCEINLINE TMap<FString, UOnlineUser*> GetMemberMap() const {return LobbyDetails.MemberList;}
	TArray<UOnlineUser*> GetMemberList() const;
	UOnlineUser* GetMember(const FString UserID);

	// Lobby-Details Setters
	FORCEINLINE void SetLobbyID(const uint64 InLobbyID) { LobbyDetails.LobbyID = FString::Printf(TEXT("%llu"), InLobbyID); }
	FORCEINLINE void SetLobbyOwnerID(const FString& InLobbyOwnerID) { LobbyDetails.LobbyOwnerID = InLobbyOwnerID; }
	void StoreMember(UOnlineUser* OnlineUser);
	void StoreMembers(TArray<UOnlineUser*> &OnlineUserList);
};

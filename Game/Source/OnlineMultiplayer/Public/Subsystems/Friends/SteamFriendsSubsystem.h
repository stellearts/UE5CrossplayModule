// Copyright © 2023 Melvin Brink

#pragma once

#pragma warning(push)
#pragma warning(disable: 4996)
#pragma warning(disable: 4265)
#include "isteamfriends.h"
#include "steam_api.h"
#pragma warning(pop)

#include "CoreMinimal.h"
#include "UserTypes.h"
#include "SteamFriendsSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSteamFriendsSubsystem, Log, All);
inline DEFINE_LOG_CATEGORY(LogSteamFriendsSubsystem);

#define CHECK_STEAM if(!SteamAPI_Init()){ UE_LOG(LogSteamFriendsSubsystem, Error, TEXT("Steam SDK is not initialized.")); return; }



/**
 * Subsystem for managing Steam friends.
 */
UCLASS(NotBlueprintable)
class ONLINEMULTIPLAYER_API USteamFriendsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

public:
	TArray<FPlatformUser> GetFriendList();
	
	void InviteToLobby(const FString& LobbyID, const FString& UserID);

private:
	STEAM_CALLBACK(USteamFriendsSubsystem, OnPersonaStateChange, PersonaStateChange_t);
	UTexture2D* CreateTextureFromAvatar(const int AvatarHandle) const;
	
	UPROPERTY()
	class ULocalUserSubsystem* LocalUserSubsystem;
	
	UPROPERTY()
	TMap<FString, FPlatformUser> FriendList;
};

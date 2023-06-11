// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "Platforms/EOS/UserTypes.h"
#include "OnlineUserSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogOnlineUserSubsystem, Log, All);
inline DEFINE_LOG_CATEGORY(LogOnlineUserSubsystem);

UENUM()
enum EUsersMapType : uint8
{
	Lobby UMETA(DisplayName="Lobby"),
	Session	UMETA(DisplayName="Session")
};



/**
 * Subsystem for managing the user.
 */
UCLASS(BlueprintType)
class ONLINEMULTIPLAYER_API UOnlineUserSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UOnlineUserSubsystem();

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

private:
	class FSteamManager* SteamManager;
	class FEosManager* EosManager;
	UPROPERTY() class USteamUserSubsystem* SteamUserSubsystem;
	UPROPERTY() class USteamOnlineUserSubsystem* SteamOnlineUserSubsystem;
	
	// Online User Maps.
	UPROPERTY() TMap<FString, UPlatformUser*> PlatformFriendList;
	UPROPERTY() TMap<FString, UEosUser*> LobbyUserList;
	UPROPERTY() TMap<FString, UEosUser*> SessionUserList;
	

public:
	// TODO: For when logged into auth-subsystem, get epic friends instead of platform, and vice versa when not.
	UPlatformUser* GetPlatformFriend(const FString& PlatformUserID);
	FORCEINLINE TMap<FString, UPlatformUser*> GetPlatformFriendList() { return PlatformFriendList; }
	bool CachePlatformFriend(const UPlatformUser* UserToStore);
	
	UEosUser* GetEosUser(const EOS_ProductUserId ProductUserID, const EUsersMapType UsersMapType);
	TMap<FString, UEosUser*> GetEosUserList(const EUsersMapType UsersMapType);
	bool CacheEosUser(UEosUser* UserToStore, const EUsersMapType UsersMapType);

	void FetchAvatar(const UEosUser* User, const TFunction<void()>& Callback);


	
	// ------------------------------- Online User Utilities -------------------------------
};

// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "Platforms/EOS/UserTypes.h"
#include "eos_sdk.h"
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

	// Online User Maps.
	FPlatformUserMap PlatformFriendList;
	FEosUserMap LobbyUserList;
	FEosUserMap SessionUserList;
	

public:
	// TODO: For when logged into auth-subsystem, get epic friends instead of platform, and vice versa when not.
	FPlatformUserPtr GetPlatformFriend(const std::string& PlatformUserID);
	FORCEINLINE FPlatformUserMap GetPlatformFriendList() { return PlatformFriendList; }
	bool CachePlatformFriend(const FPlatformUserPtr& UserToStore);
	
	FEosUserPtr GetEosUser(const EOS_ProductUserId ProductUserID, const EUsersMapType UsersMapType);
	FEosUserMap GetEosUserList(const EUsersMapType UsersMapType);
	bool CacheEosUser(const FEosUserPtr& UserToStore, const EUsersMapType UsersMapType);


	
	// ------------------------------- Online User Utilities -------------------------------
};

// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "UserTypes.h"
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
 * Subsystem for managing user's from the friend, lobby or session lists.
 *
 * Provides helper functions for getting certain user data.
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
	UPROPERTY() class USteamLocalUserSubsystem* SteamLocalUserSubsystem;
	UPROPERTY() class USteamOnlineUserSubsystem* SteamOnlineUserSubsystem;
	
	// Online User Maps.
	UPROPERTY() TMap<FString, UPlatformUser*> FriendList;
	UPROPERTY() TMap<FString, UEosUser*> SessionList;

public:
	// TODO: For when logged into auth-subsystem, get epic friends instead of platform, and vice versa when not.
	
	// Get single
	UPlatformUser* GetFriend(const FString& PlatformUserID);
	UEosUser* GetSessionMember(const FString& ProductUserID);
	
	// Get all
	FORCEINLINE TMap<FString, UPlatformUser*> GetFriendList() { return FriendList; }
	FORCEINLINE TMap<FString, UEosUser*> GetSessionList() { return SessionList; }

	// Store single
	bool StoreFriend(UPlatformUser* PlatformUser);
	bool StoreSessionMember(UEosUser* EosUser);

	// Store multiple
	bool StoreFriends(TArray<UPlatformUser*> PlatformUsers);
	bool StoreSessionMembers(TArray<UEosUser*> EosUsers);


	
	// --- Utilities ---
	
	void FetchAvatar(const FString& UserID, EOS_EExternalAccountType PlatformType, const TFunction<void>& Callback) const;
};

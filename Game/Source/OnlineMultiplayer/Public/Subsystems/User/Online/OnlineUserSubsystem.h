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
	UPROPERTY() TMap<FString, FPlatformUser> FriendList;
	UPROPERTY() TMap<FString, UOnlineUser*> SessionList;

public:
	// Get single
	FPlatformUser GetFriend(const FString& PlatformUserID);
	
	// Get all
	FORCEINLINE TMap<FString, FPlatformUser> GetFriendList() { return FriendList; }

	// Store single
	bool StoreFriend(const FPlatformUser& PlatformUser);

	// Store multiple
	bool StoreFriends(TArray<FPlatformUser> PlatformUsers);


	
	// --- Utilities ---
	
	void FetchAvatar(const FString& UserID, EOS_EExternalAccountType PlatformType, const TFunction<void>& Callback) const;
};

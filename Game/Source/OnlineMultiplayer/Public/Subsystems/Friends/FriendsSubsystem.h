// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "eos_sdk.h"
#include "Types/UserTypes.h"
#include "FriendsSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogFriendsSubsystem, Log, All);
inline DEFINE_LOG_CATEGORY(LogFriendsSubsystem);



/**
 * Subsystem for managing friends.
 */
UCLASS(BlueprintType)
class ONLINEMULTIPLAYER_API UFriendsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
protected:
	UFriendsSubsystem();
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

public:
	UFUNCTION(BlueprintCallable)
	TArray<FPlatformUser> GetFriendList() const;

	UFUNCTION(BlueprintCallable)
	void InviteToLobby(const FPlatformUser& PlatformUser) const;

private:
	UPROPERTY()
	TMap<FString, UOnlineUser*> EosFriendList;
	
	class FEosManager* EosManager;
	EOS_HFriends FriendsHandle;

	UPROPERTY() class USteamFriendsSubsystem* SteamFriendsSubsystem;
	UPROPERTY() class ULocalUserSubsystem* LocalUserSubsystem;
};

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
	Friends	UMETA(DisplayName="Friends"),
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
	class FSteamManager& SteamManager;
	class FEosManager& EosManager;
	UPROPERTY() class USteamUserSubsystem* SteamUserSubsystem;

	// Online User Maps.
	FUsersMap FriendsList; // TODO: Separate platform friends list? Separate map for each platform and finding based on that platforms user id?
	FUsersMap LobbyUserList;
	FUsersMap SessionUserList;
	

public:
	UUser* GetUser(const EOS_ProductUserId ProductUserID, const EUsersMapType UsersMapType);
	bool AddUser(UUser* UserToStore, const EUsersMapType UsersMapType);


	
	// ------------------------------- Online User Utilities -------------------------------
};

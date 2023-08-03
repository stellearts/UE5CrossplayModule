// Copyright © 2023 Melvin Brink


#include "Subsystems/User/Online/OnlineUserSubsystem.h"
#include "Subsystems/User/Online/SteamOnlineUserSubsystem.h"
#include "EOSManager.h"
#include "SteamManager.h"
#include "eos_common.h"



UOnlineUserSubsystem::UOnlineUserSubsystem() : SteamManager(&FSteamManager::Get()), EosManager(&FEosManager::Get())
{
}

void UOnlineUserSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	SteamOnlineUserSubsystem = Collection.InitializeDependency<USteamOnlineUserSubsystem>();
}


// --------------------------------


/**
 * Returns a friend from the user's platform and also from Epic if logged into the auth-interface.
 *
 * @param PlatformUserID The Platform-User-ID of the user to get.
 */
FPlatformUser UOnlineUserSubsystem::GetFriend(const FString& PlatformUserID)
{
	FPlatformUser* FoundUserPtr = FriendList.Find(PlatformUserID);
	if(!FoundUserPtr) return FPlatformUser();
	return *FoundUserPtr;
}


// --------------------------------


/**
 * Tries to store the given user in the friend-list.
 *
 * @param PlatformUser The Platform-User to store in the friend-list.
 *
 * @return boolean, true if successful
 */
bool UOnlineUserSubsystem::StoreFriend(const FPlatformUser& PlatformUser)
{
	if(PlatformUser.UserID.IsEmpty())
	{
		UE_LOG(LogOnlineUserSubsystem, Warning, TEXT("Invalid User provided."));
		return false;
	}
	
	FriendList.Add(PlatformUser.UserID, PlatformUser);
	return true;
}


// --------------------------------


/**
 * Tries to store the given list of user's in the friend-list.
 *
 * @param PlatformUsers The Platform-Users to store in the friend-list.
 */
bool UOnlineUserSubsystem::StoreFriends(TArray<FPlatformUser> PlatformUsers)
{
	for (const auto PlatformUser : PlatformUsers)
	{
		StoreFriend(PlatformUser);
	}
	return true;
}


// -------------------------------- Utilities --------------------------------


void UOnlineUserSubsystem::FetchAvatar(const FString& UserID, const EOS_EExternalAccountType PlatformType, const TFunction<void>& Callback) const
{
	if(UserID.IsEmpty())
	{
		UE_LOG(LogOnlineUserSubsystem, Warning, TEXT("Invalid User-ID given in UOnlineUserSubsystem::FetchAvatar"));
		return;
	}

	if(PlatformType == EOS_EExternalAccountType::EOS_EAT_STEAM)
	{
		SteamOnlineUserSubsystem->FetchAvatar(UserID, Callback);
	}
}
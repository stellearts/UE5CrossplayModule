// Copyright © 2023 Melvin Brink

#include "Platforms/EOS/Subsystems/OnlineUserSubsystem.h"
#include "Platforms/EOS/EOSManager.h"
#include "Platforms/Steam/SteamManager.h"
#include "Platforms/Steam/Subsystems/SteamOnlineUserSubsystem.h"

#include "eos_common.h"
#include "Helpers.h"


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
UPlatformUser* UOnlineUserSubsystem::GetFriend(const FString& PlatformUserID)
{
	UPlatformUser** FoundUser = FriendList.Find(PlatformUserID);
	return FoundUser ? *FoundUser : nullptr;
}


/**
 * Returns a session-member from the list if exists.
 *
 * @param ProductUserID The Product-User-ID of the member to find.
 */
UEosUser* UOnlineUserSubsystem::GetSessionMember(const FString& ProductUserID)
{
	UEosUser** FoundUser = SessionList.Find(ProductUserID);
	return FoundUser ? *FoundUser : nullptr;
}


// --------------------------------


/**
 * Tries to store the given user in the friend-list.
 *
 * @param PlatformUser The Platform-User to store in the friend-list.
 *
 * @return boolean, true if successful
 */
bool UOnlineUserSubsystem::StoreFriend(UPlatformUser* PlatformUser)
{
	if(!PlatformUser)
	{
		UE_LOG(LogOnlineUserSubsystem, Warning, TEXT("Invalid User provided."));
		return false;
	}
	
	FriendList.Add(PlatformUser->GetPlatformID(), PlatformUser);
	return true;
}


/**
 * Tries to store the given EosUser in the session-list.
 *
 * @param EosUser The Eos-User to store in the session-list.
 *
 * @return boolean, true if successful
 */
bool UOnlineUserSubsystem::StoreSessionMember(UEosUser* EosUser)
{
	if(!EosUser)
	{
		UE_LOG(LogOnlineUserSubsystem, Warning, TEXT("Invalid User provided."));
		return false;
	}
	
	const FString ProductUserID = EosUser->GetProductUserID();
	if(ProductUserID.IsEmpty())
	{
		UE_LOG(LogOnlineUserSubsystem, Warning, TEXT("Provided User is missing its ProductUserID"));
		return false;
	}
	
	SessionList.Add(ProductUserID, EosUser);
	return true;
}


// --------------------------------


/**
 * Tries to store the given list of user's in the friend-list.
 *
 * @param PlatformUsers The Platform-Users to store in the friend-list.
 */
bool UOnlineUserSubsystem::StoreFriends(TArray<UPlatformUser*> PlatformUsers)
{
	for (const auto PlatformUser : PlatformUsers)
	{
		StoreFriend(PlatformUser);
	}
	return true;
}


/**
 * Tries to store the given list of user's in the session-list.
 *
 * @param EosUsers The Eos-Users to store in the session-list.
 */
bool UOnlineUserSubsystem::StoreSessionMembers(TArray<UEosUser*> EosUsers)
{
	for (const auto EosUser : EosUsers)
	{
		StoreFriend(EosUser);
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
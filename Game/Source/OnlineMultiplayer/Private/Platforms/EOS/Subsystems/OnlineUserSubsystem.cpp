// Copyright © 2023 Melvin Brink

#include "Platforms/EOS/Subsystems/OnlineUserSubsystem.h"
#include "Platforms/EOS/EOSManager.h"
#include "Platforms/Steam/SteamManager.h"
#include "Platforms/Steam/Subsystems/SteamOnlineUserSubsystem.h"

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
 * @param PlatformUserID The Platform User ID of the user to get.
 */
UPlatformUser* UOnlineUserSubsystem::GetPlatformFriend(const FString& PlatformUserID)
{
	// FPlatformUserPtr* FoundUser = PlatformFriendList.Find(FString(PlatformUserID.c_str()));
	// return FoundUser ? *FoundUser : nullptr;
	return nullptr;
}


/**
 * Tries to cache the given user.
 *
 * @param UserToStore The Platform-User to store.
 */
bool UOnlineUserSubsystem::CachePlatformFriend(const UPlatformUser* UserToStore)
{
	if(!UserToStore)
	{
		UE_LOG(LogOnlineUserSubsystem, Warning, TEXT("Invalid User provided."));
		return false;
	}

	// const std::string PlatformUserID = UserToStore->GetPlatformID();
	// if(!PlatformUserID.length())
	// {
	// 	UE_LOG(LogOnlineUserSubsystem, Warning, TEXT("Provided User is missing its PlatformUserID"));
	// 	return false;
	// }
	//
	// PlatformFriendList.Add(FString(PlatformUserID.c_str()), UserToStore);
	return true;
}


// --------------------------------


/**
 * Returns a user containing eos user info from the specified map type.
 *
 * @param ProductUserID The EOS Product User ID of the user to get.
 * @param UsersMapType The list where to get the user from.
 */
UEosUser* UOnlineUserSubsystem::GetEosUser(const EOS_ProductUserId ProductUserID, const EUsersMapType UsersMapType)
{
	// FoundUser is a double pointer since the .Find method returns a pointer to the value inside.
	UEosUser** FoundUser = nullptr;
	switch (UsersMapType)
	{
	case Lobby:
		FoundUser = LobbyUserList.Find(EosIDToString(ProductUserID));
		break;

	case Session:
		FoundUser = SessionUserList.Find(EosIDToString(ProductUserID));
		break;

	default:
		UE_LOG(LogOnlineUserSubsystem, Warning, TEXT("Invalid UsersMapType provided."));
		break;
	}
	
	return FoundUser ? *FoundUser : nullptr;
}


/**
 * Returns a the user list of the given type.
 * 
 * @param UsersMapType The list where to get the users from.
 */
TMap<FString, UEosUser*> UOnlineUserSubsystem::GetEosUserList(const EUsersMapType UsersMapType)
{
	switch (UsersMapType)
	{
	case Lobby:
		return LobbyUserList;

	case Session:
		return SessionUserList;

	default:
		UE_LOG(LogOnlineUserSubsystem, Warning, TEXT("Invalid UsersMapType provided."));
		return TMap<FString, UEosUser*>();
	}
}


/**
 * Tries to cache the given user in the given map type.
 *
 * @param UserToStore The EOS-User to store.
 * @param UsersMapType The list where to store the user in.
 */
bool UOnlineUserSubsystem::CacheEosUser(UEosUser* UserToStore, const EUsersMapType UsersMapType)
{
	if(!UserToStore)
	{
		UE_LOG(LogOnlineUserSubsystem, Warning, TEXT("Invalid User provided."));
		return false;
	}
	
	const FString ProductUserID = EosIDToString(UserToStore->GetProductUserID());
	if(ProductUserID.IsEmpty())
	{
		UE_LOG(LogOnlineUserSubsystem, Warning, TEXT("Provided User is missing its ProductUserID"));
		return false;
	}
	
	switch (UsersMapType)
	{
	case Lobby:
		LobbyUserList.Add(ProductUserID, UserToStore);
		break;

	case Session:
		SessionUserList.Add(ProductUserID, UserToStore);
		break;

	default:
		UE_LOG(LogOnlineUserSubsystem, Warning, TEXT("Invalid UsersMapType provided."));
		return false;
	}

	return true;
}


// --------------------------------


void UOnlineUserSubsystem::FetchAvatar(const UEosUser* User, const TFunction<void()>& Callback)
{
	if(!User)
	{
		UE_LOG(LogOnlineUserSubsystem, Warning, TEXT("Invalid user ptr given in UOnlineUserSubsystem::FetchAvatar"));
		return;
	}

	if(User->GetPlatform() == EOS_EExternalAccountType::EOS_EAT_STEAM)
	{
		// SteamOnlineUserSubsystem->FetchAvatar(User->GetPlatformID(), Callback);
	}
}
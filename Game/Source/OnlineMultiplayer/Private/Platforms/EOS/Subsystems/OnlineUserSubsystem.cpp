// Copyright © 2023 Melvin Brink

#include "Platforms/EOS/Subsystems/OnlineUserSubsystem.h"
#include "Platforms/EOS/EOSManager.h"
#include "Platforms/Steam/SteamManager.h"
#include "Platforms/Steam/Subsystems/SteamUserSubsystem.h"



UOnlineUserSubsystem::UOnlineUserSubsystem()
{
}

void UOnlineUserSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}


// --------------------------------


/**
 * Returns a user containing online user info from the specified map type.
 *
 * @param ProductUserID The EOS-Product-User-ID of the user to get.
 * @param UsersMapType The list where to get the user from.
 */
UUser* UOnlineUserSubsystem::GetUser(const EOS_ProductUserId ProductUserID, const EUsersMapType UsersMapType)
{
	// Double pointer since the .Find method returns a pointer to the value inside, which is also a pointer.
	// This is needed so that I can share the same user instance between multiple maps and keep them the same.
	UUser** FoundUser = nullptr;
	switch (UsersMapType)
	{
	case Friends:
		FoundUser = FriendsList.Find(ProductUserID);
		break;

	case Lobby:
		FoundUser = LobbyUserList.Find(ProductUserID);
		break;

	case Session:
		FoundUser = SessionUserList.Find(ProductUserID);
		break;

	default:
		UE_LOG(LogOnlineUserSubsystem, Warning, TEXT("Invalid UsersMapType provided."));
		break;
	}
	
	return FoundUser ? *FoundUser : nullptr;
}


/**
 * Tries to store the given user in the given map type.
 *
 * @param UserToStore The user to store.
 * @param UsersMapType The list where to store the user in.
 */
bool UOnlineUserSubsystem::AddUser(UUser* UserToStore, const EUsersMapType UsersMapType)
{
	if(!UserToStore)
	{
		UE_LOG(LogOnlineUserSubsystem, Warning, TEXT("Invalid User provided."));
		return false;
	}

	// TODO: This method would be incompatible with users from the SteamUsersMap and other platforms only since they wont have an EOS_ProductUserId.
	const EOS_ProductUserId ProductUserId = UserToStore->GetProductUserID();
	switch (UsersMapType)
	{
	case Friends:
		FriendsList.Add(ProductUserId, UserToStore);
		break;

	case Lobby:
		LobbyUserList.Add(ProductUserId, UserToStore);
		break;

	case Session:
		SessionUserList.Add(ProductUserId, UserToStore);
		break;

	default:
		UE_LOG(LogOnlineUserSubsystem, Warning, TEXT("Invalid UsersMapType provided."));
		return false;
	}

	return true;
}

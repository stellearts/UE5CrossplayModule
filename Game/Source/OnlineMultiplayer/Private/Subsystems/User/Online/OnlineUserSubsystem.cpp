// Copyright © 2023 Melvin Brink


#include "Subsystems/User/Online/OnlineUserSubsystem.h"
#include "Subsystems/User/Online/SteamOnlineUserSubsystem.h"
#include "EOSManager.h"
#include "SteamManager.h"
#include "eos_common.h"
#include "Subsystems/Connect/ConnectSubsystem.h"
#include "Subsystems/User/Local/LocalUserSubsystem.h"


UOnlineUserSubsystem::UOnlineUserSubsystem() : SteamManager(&FSteamManager::Get()), EosManager(&FEosManager::Get())
{
}

void UOnlineUserSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	SteamOnlineUserSubsystem = Collection.InitializeDependency<USteamOnlineUserSubsystem>();
}


// -------------------------------- Utilities --------------------------------


/*
 * Returns a user with all necessary properties if exists.
 * Requires a callback since it will be an asynchronous operation when calling this for the first time, user's are cached after completion.
 */
void UOnlineUserSubsystem::GetOnlineUser(const FString& ProductUserID, const TFunction<void(FGetOnlineUserResult)> &Callback)
{
	if(UOnlineUser** OnlineUser = CachedOnlineUsers.Find(ProductUserID); OnlineUser && *OnlineUser)
	{
		UE_LOG(LogOnlineUserSubsystem, Log, TEXT("User is cached, skipping fetch for this user."))
		Callback(FGetOnlineUserResult{*OnlineUser, EGetOnlineUserResultCode::Success});
		return;
	}

	TArray<FString> ProductUserIDList;
	ProductUserIDList.Add(ProductUserID);

	// Get the information about the user, cache, and call the callback.
	UConnectSubsystem* ConnectSubsystem = GetGameInstance()->GetSubsystem<UConnectSubsystem>();
	ConnectSubsystem->GetOnlineUserDetails(ProductUserIDList, [this, Callback](const TArray<UOnlineUser*>& OnlineUserList)
	{
		if(OnlineUserList.IsEmpty())
		{
			UE_LOG(LogOnlineUserSubsystem, Error, TEXT("Failed to get the user's details in UOnlineUserSubsystem::GetOnlineUser"))
			Callback(FGetOnlineUserResult{nullptr, EGetOnlineUserResultCode::Failed});
		}
		UOnlineUser* OutOnlineUser = OnlineUserList[0];
		CachedOnlineUsers.Add(OutOnlineUser->GetProductUserID(), OutOnlineUser);
		Callback(FGetOnlineUserResult{OutOnlineUser, EGetOnlineUserResultCode::Success});
	});
}

/*
 * Returns a list of users with all necessary properties if they exist.
 * Requires a callback since it will be an asynchronous operation when certain users are not cached yet.
 */
void UOnlineUserSubsystem::GetOnlineUsers(TArray<FString>& ProductUserIDs, const TFunction<void(FGetOnlineUsersResult)> &Callback)
{
	TArray<UOnlineUser*> OutOnlineUsers;
	TArray<FString> ProductUserIDsToFetch;
	for (FString& ProductUserID : ProductUserIDs)
	{
		if(UOnlineUser** OnlineUser = CachedOnlineUsers.Find(ProductUserID); OnlineUser && *OnlineUser)
		{
			UE_LOG(LogOnlineUserSubsystem, Log, TEXT("User is cached, skipping fetch for this user."))
			OutOnlineUsers.Add(*OnlineUser);
		}
		else ProductUserIDsToFetch.Add(ProductUserID);
	}

	// Done if all user's were cached
	if(!ProductUserIDsToFetch.Num())
	{
		Callback(FGetOnlineUsersResult{OutOnlineUsers, EGetOnlineUserResultCode::Success});
		return;
	}

	// Get the information about the users, cache them, and call the callback.
	UConnectSubsystem* ConnectSubsystem = GetGameInstance()->GetSubsystem<UConnectSubsystem>();
	ConnectSubsystem->GetOnlineUserDetails(ProductUserIDsToFetch, [this, OutOnlineUsers, Callback](const TArray<UOnlineUser*>& OnlineUserList)
	{
		if(OnlineUserList.IsEmpty())
		{
			UE_LOG(LogOnlineUserSubsystem, Error, TEXT("Failed to get the details of on or more users in UOnlineUserSubsystem::GetOnlineUsers"))
			Callback(FGetOnlineUsersResult{OutOnlineUsers, EGetOnlineUserResultCode::Failed});
		}
		

		for (UOnlineUser* OnlineUser : OnlineUserList) CachedOnlineUsers.Add(OnlineUser->GetProductUserID(), OnlineUser);
		Callback(FGetOnlineUsersResult{OnlineUserList, EGetOnlineUserResultCode::Success});
	});
}

void UOnlineUserSubsystem::LoadUserAvatar(const UOnlineUser* OnlineUser, const TFunction<void>& Callback)
{
	
}

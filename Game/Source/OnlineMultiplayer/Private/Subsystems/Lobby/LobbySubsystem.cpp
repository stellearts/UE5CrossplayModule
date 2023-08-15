// Copyright © 2023 Melvin Brink

#include "Subsystems/Lobby/LobbySubsystem.h"
#include "Subsystems/Lobby/SteamLobbySubsystem.h"
#include "Subsystems/Connect/ConnectSubsystem.h"
#include "Subsystems/User/Local/LocalUserSubsystem.h"
#include "Subsystems/User/Online/OnlineUserSubsystem.h"
#include "EOSManager.h"
#include "UserTypes.h"
#include "Helpers.h"
#include "eos_lobby.h"



ULobbySubsystem::ULobbySubsystem() : EosManager(&FEosManager::Get())
{
	
}

void ULobbySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	OnlineUserSubsystem = Collection.InitializeDependency<UOnlineUserSubsystem>();
	LocalUserSubsystem = Collection.InitializeDependency<ULocalUserSubsystem>();
	
	switch (LocalUserSubsystem->GetLocalUser()->GetPlatform())
	{
	case EPlatform::Steam:
		LocalPlatformLobbySubsystem = Collection.InitializeDependency<USteamLobbySubsystem>();
		break;
	default:
		break;
	}
	
	const EOS_HPlatform PlatformHandle = EosManager->GetPlatformHandle();
	if(!PlatformHandle) return;
	LobbyHandle = EOS_Platform_GetLobbyInterface(PlatformHandle);
	
	// Register lobby member status update callback.
	EOS_Lobby_AddNotifyLobbyMemberStatusReceivedOptions Options;
	Options.ApiVersion = EOS_LOBBY_ADDNOTIFYLOBBYMEMBERSTATUSRECEIVED_API_LATEST;
	OnLobbyMemberStatusUpdateID = EOS_Lobby_AddNotifyLobbyMemberStatusReceived(LobbyHandle, &Options, this, &ULobbySubsystem::OnLobbyMemberStatusUpdate);
}


// --------------------------------------------


void ULobbySubsystem::CreateLobby(const int32 MaxMembers)
{
	if(InLobby())
	{
		OnCreateLobbyCompleteDelegate.Broadcast(ECreateLobbyResultCode::InLobby, LobbyDetails);
		return;
	}

	// Lobby Settings
	EOS_Lobby_CreateLobbyOptions CreateLobbyOptions;
	CreateLobbyOptions.ApiVersion = EOS_LOBBY_CREATELOBBY_API_LATEST;
	CreateLobbyOptions.LocalUserId = EosProductIDFromString(LocalUserSubsystem->GetLocalUser()->GetProductUserID());
	CreateLobbyOptions.MaxLobbyMembers = MaxMembers;
	CreateLobbyOptions.PermissionLevel = EOS_ELobbyPermissionLevel::EOS_LPL_INVITEONLY;
	CreateLobbyOptions.bPresenceEnabled = true;
	CreateLobbyOptions.bAllowInvites = true;
	CreateLobbyOptions.BucketId = "PresenceLobby"; // TODO: What is this?
	CreateLobbyOptions.bDisableHostMigration = false;
	CreateLobbyOptions.bEnableRTCRoom = false;
	CreateLobbyOptions.LocalRTCOptions = nullptr;
	CreateLobbyOptions.LobbyId = nullptr;
	CreateLobbyOptions.bEnableJoinById = true;
	CreateLobbyOptions.bRejoinAfterKickRequiresInvite = true;
	
	// ClientData to pass to the callback
	struct FCreateLobbyClientData
	{
		ULobbySubsystem* Self;
		int32 MaxMembers;
	};
	FCreateLobbyClientData* CreateLobbyClientData = new FCreateLobbyClientData();
	CreateLobbyClientData->Self = this;
	CreateLobbyClientData->MaxMembers = MaxMembers;

	// Create the EOS lobby and handle the result
	EOS_Lobby_CreateLobby(LobbyHandle, &CreateLobbyOptions, CreateLobbyClientData, [](const EOS_Lobby_CreateLobbyCallbackInfo* Data)
    {
		const FCreateLobbyClientData* ClientData = static_cast<FCreateLobbyClientData*>(Data->ClientData);
        ULobbySubsystem* LobbySubsystem = ClientData->Self;
		ULocalUser* LocalUser = LobbySubsystem->LocalUserSubsystem->GetLocalUser();

		// EOS_LobbyId to FString
		FString LobbyID;
		if(Data->LobbyId) LobbyID = FString(Data->LobbyId);
		
		if(Data->ResultCode == EOS_EResult::EOS_Success)
		{
			// Set lobby-details manually since we have that info locally
			LobbySubsystem->LobbyDetails.LobbyID = LobbyID;
			LobbySubsystem->LobbyDetails.LobbyOwnerID = LocalUser->GetProductUserID();
			LobbySubsystem->LobbyDetails.MemberList = TMap<FString, UOnlineUser*>();
			LobbySubsystem->LobbyDetails.MaxMembers = ClientData->MaxMembers;
			LobbySubsystem->OnCreateLobbyCompleteDelegate.Broadcast(ECreateLobbyResultCode::Success, LobbySubsystem->LobbyDetails);

			UE_LOG(LogTemp, Log, TEXT("LobbyDetails.LobbyID: %s"), *LobbySubsystem->LobbyDetails.LobbyID)
			
			// Create a shadow lobby for the user's local-platform besides the EOS lobby to be able to easily invite players on that platform.
			if(LocalUser->GetPlatform() == EPlatform::Epic) return;
			LobbySubsystem->LocalPlatformLobbySubsystem->OnCreateShadowLobbyCompleteDelegate.BindUObject(LobbySubsystem, &ULobbySubsystem::OnCreateShadowLobbyComplete);
			LobbySubsystem->LocalPlatformLobbySubsystem->CreateLobby();
		}
		else if(Data->ResultCode == EOS_EResult::EOS_Lobby_PresenceLobbyExists)
		{
			// If there already is an existing 'Presence Lobby' you joined. You can then join this lobby using your User-ID
			LobbySubsystem->OnCreateLobbyCompleteDelegate.Broadcast(ECreateLobbyResultCode::PresenceLobbyExists, LobbySubsystem->LobbyDetails);
		}
		else
		{
			UE_LOG(LogLobbySubsystem, Warning, TEXT("Failed to create Lobby. Code: [%s]"), *FString(EOS_EResult_ToString(Data->ResultCode)));
			LobbySubsystem->OnCreateLobbyCompleteDelegate.Broadcast(ECreateLobbyResultCode::Unknown, LobbySubsystem->LobbyDetails);
		}
		
		delete ClientData;
		// End Lambda.
    });
}


// --------------------------------------------


void ULobbySubsystem::JoinLobbyByID(const FString& LobbyID)
{
	// Create the Lobby Search Handle and set the Options
    EOS_Lobby_CreateLobbySearchOptions LobbySearchOptions;
    LobbySearchOptions.ApiVersion = EOS_LOBBY_CREATELOBBYSEARCH_API_LATEST;
    LobbySearchOptions.MaxResults = 1;
    if(EOS_Lobby_CreateLobbySearch(LobbyHandle, &LobbySearchOptions, &LobbySearchByLobbyIDHandle) != EOS_EResult::EOS_Success)
    {
        UE_LOG(LogLobbySubsystem, Error, TEXT("Failed to create lobby search handle."));
        return;
    }
	
    EOS_LobbySearch_SetLobbyIdOptions SetLobbyIdOptions;
    SetLobbyIdOptions.ApiVersion = EOS_LOBBYSEARCH_SETLOBBYID_API_LATEST;
    SetLobbyIdOptions.LobbyId = TCHAR_TO_UTF8(*LobbyID);
    if(EOS_LobbySearch_SetLobbyId(LobbySearchByLobbyIDHandle, &SetLobbyIdOptions) != EOS_EResult::EOS_Success)
    {
	    UE_LOG(LogLobbySubsystem, Error, TEXT("Failed to create lobby search handle."));
    	return;
    }
	
    // Find the lobby.
    EOS_LobbySearch_FindOptions FindOptions;
    FindOptions.ApiVersion = EOS_LOBBYSEARCH_FIND_API_LATEST;
	FindOptions.LocalUserId = EosProductIDFromString(LocalUserSubsystem->GetLocalUser()->GetProductUserID());
	EOS_LobbySearch_Find(LobbySearchByLobbyIDHandle, &FindOptions, this, [] (const EOS_LobbySearch_FindCallbackInfo* Data) {
		if (Data->ResultCode == EOS_EResult::EOS_Success)
		{
			ULobbySubsystem* LobbySubsystem = static_cast<ULobbySubsystem*>(Data->ClientData);

			// Retrieve lobby details handle
			EOS_HLobbyDetails LobbyDetailsHandle;
			constexpr EOS_LobbySearch_CopySearchResultByIndexOptions CopyOptions = { EOS_LOBBYSEARCH_COPYSEARCHRESULTBYINDEX_API_LATEST, 0 };
			if (EOS_LobbySearch_CopySearchResultByIndex(LobbySubsystem->LobbySearchByLobbyIDHandle, &CopyOptions, &LobbyDetailsHandle) == EOS_EResult::EOS_Success)
			{
				// Try to join the lobby using the details handle.
				LobbySubsystem->JoinLobbyByHandle(LobbyDetailsHandle);
			}
			else
				UE_LOG(LogLobbySubsystem, Error, TEXT("Failed to retrieve the lobby details."));
		}
		else
			UE_LOG(LogLobbySubsystem, Warning, TEXT("Failed to find a lobby with Result Code: [%s]"), *FString(EOS_EResult_ToString(Data->ResultCode)));
	});
}

void ULobbySubsystem::JoinLobbyByUserID(const FString& UserID)
{
	// Create the lobby search handle.
	EOS_Lobby_CreateLobbySearchOptions LobbySearchOptions;
	LobbySearchOptions.ApiVersion = EOS_LOBBY_CREATELOBBYSEARCH_API_LATEST;
	LobbySearchOptions.MaxResults = 1;
	
	if(EOS_Lobby_CreateLobbySearch(LobbyHandle, &LobbySearchOptions, &LobbySearchByUserIDHandle) != EOS_EResult::EOS_Success)
	{
		UE_LOG(LogLobbySubsystem, Error, TEXT("Failed to create lobby search handle."));
		OnJoinLobbyCompleteDelegate.Broadcast(EJoinLobbyResultCode::EosFailure, LobbyDetails);
		return;
	}

	// Set the User ID to search for.
	EOS_LobbySearch_SetTargetUserIdOptions SetTargetUserIdOptions;
	SetTargetUserIdOptions.ApiVersion = EOS_LOBBYSEARCH_SETTARGETUSERID_API_LATEST;
	SetTargetUserIdOptions.TargetUserId = EosProductIDFromString(UserID);
	EOS_LobbySearch_SetTargetUserId(LobbySearchByUserIDHandle, &SetTargetUserIdOptions);
	
	EOS_LobbySearch_FindOptions LobbySearchFindOptions;
	LobbySearchFindOptions.ApiVersion = EOS_LOBBYSEARCH_FIND_API_LATEST;
	LobbySearchFindOptions.LocalUserId = EosProductIDFromString(LocalUserSubsystem->GetLocalUser()->GetProductUserID());

	EOS_LobbySearch_Find(LobbySearchByUserIDHandle, &LobbySearchFindOptions, this, [](const EOS_LobbySearch_FindCallbackInfo* Data)
	{
		ULobbySubsystem* LobbySubsystem = static_cast<ULobbySubsystem*>(Data->ClientData);
		if(Data->ResultCode == EOS_EResult::EOS_Success)
		{

			// Create the search results options and set it to find the first lobby found, should only be one.
			EOS_LobbySearch_CopySearchResultByIndexOptions Options;
			Options.ApiVersion = EOS_LOBBYSEARCH_COPYSEARCHRESULTBYINDEX_API_LATEST;
			Options.LobbyIndex = 0;
		
			EOS_HLobbyDetails LobbyDetailsHandle;
			const EOS_EResult SearchResultCode = EOS_LobbySearch_CopySearchResultByIndex(LobbySubsystem->LobbySearchByUserIDHandle, &Options, &LobbyDetailsHandle);
			if(SearchResultCode == EOS_EResult::EOS_Success)
			{
				LobbySubsystem->JoinLobbyByHandle(LobbyDetailsHandle);
			}
			else
			{
				UE_LOG(LogLobbySubsystem, Warning, TEXT("EOS_LobbySearch_CopySearchResultByIndex Failed. Result-Code: [%s]"), *FString(EOS_EResult_ToString(SearchResultCode)));
				LobbySubsystem->OnJoinLobbyCompleteDelegate.Broadcast(EJoinLobbyResultCode::Unknown, LobbySubsystem->LobbyDetails);
			}
		}
		else if(Data->ResultCode == EOS_EResult::EOS_NotFound)
		{
			UE_LOG(LogLobbySubsystem, Log, TEXT("No lobbies found."));
			LobbySubsystem->OnJoinLobbyCompleteDelegate.Broadcast(EJoinLobbyResultCode::NotFound, LobbySubsystem->LobbyDetails);
		}
		else
		{
			UE_LOG(LogLobbySubsystem, Warning, TEXT("Failed to find a lobby. Result-Code: [%s]"), *FString(EOS_EResult_ToString(Data->ResultCode)));
			LobbySubsystem->OnJoinLobbyCompleteDelegate.Broadcast(EJoinLobbyResultCode::Unknown, LobbySubsystem->LobbyDetails);
		}
	});
}

void ULobbySubsystem::LeaveLobby()
{
	if(!InLobby() || !LobbyHandle)
	{
		UE_LOG(LogLobbySubsystem, Error, TEXT("Cannot leave a lobby when not in one."));
		OnLeaveLobbyCompleteDelegate.Broadcast(ELeaveLobbyResultCode::NotInLobby);
		return;
	}
	
	EOS_Lobby_LeaveLobbyOptions LeaveLobbyOptions;
	LeaveLobbyOptions.ApiVersion = EOS_LOBBY_LEAVELOBBY_API_LATEST;
	LeaveLobbyOptions.LocalUserId = EosProductIDFromString(LocalUserSubsystem->GetLocalUser()->GetProductUserID());
	LeaveLobbyOptions.LobbyId = TCHAR_TO_UTF8(*LobbyDetails.LobbyID);
	
	EOS_Lobby_LeaveLobby(LobbyHandle, &LeaveLobbyOptions, this, [](const EOS_Lobby_LeaveLobbyCallbackInfo* Data)
	{
		ULobbySubsystem* LobbySubsystem = static_cast<ULobbySubsystem*>(Data->ClientData);
		if(Data->ResultCode == EOS_EResult::EOS_Success)
		{
			// Leave shadow lobby as well.
			LobbySubsystem->LocalPlatformLobbySubsystem->LeaveLobby();

			// Clear lobby details and broadcast success.
			LobbySubsystem->LobbyDetails.Reset();
			LobbySubsystem->OnLeaveLobbyCompleteDelegate.Broadcast(ELeaveLobbyResultCode::Success);
		}
		else
		{
			UE_LOG(LogLobbySubsystem, Error, TEXT("Failed to leave the lobby. Result-Code: [%s]"), *FString(EOS_EResult_ToString(Data->ResultCode)))
			LobbySubsystem->OnLeaveLobbyCompleteDelegate.Broadcast(ELeaveLobbyResultCode::Failure);
		}
	});
}

/**
 * Tries to join the lobby using the given handle.
 *
 * Will release the given handle from memory after completion.
 */
void ULobbySubsystem::JoinLobbyByHandle(const EOS_HLobbyDetails& LobbyDetailsHandle)
{
	EOS_Lobby_JoinLobbyOptions JoinOptions;
	JoinOptions.ApiVersion = EOS_LOBBY_JOINLOBBY_API_LATEST;
	JoinOptions.LobbyDetailsHandle = LobbyDetailsHandle;
	JoinOptions.LocalUserId = EosProductIDFromString(LocalUserSubsystem->GetLocalUser()->GetProductUserID());
	JoinOptions.bPresenceEnabled = true;
	JoinOptions.LocalRTCOptions = nullptr;
    
	EOS_Lobby_JoinLobby(LobbyHandle, &JoinOptions, this, &ULobbySubsystem::OnJoinLobbyComplete);

	// Release the handle after using it
	EOS_LobbyDetails_Release(LobbyDetailsHandle);
}

void ULobbySubsystem::OnJoinLobbyComplete(const EOS_Lobby_JoinLobbyCallbackInfo* Data)
{
	ULobbySubsystem* LobbySubsystem = static_cast<ULobbySubsystem*>(Data->ClientData);
	ULocalUser* LocalUser = LobbySubsystem->LocalUserSubsystem->GetLocalUser();

	if(Data->ResultCode == EOS_EResult::EOS_Success || Data->ResultCode == EOS_EResult::EOS_Lobby_PresenceLobbyExists)
	{
		// Successfully joined the lobby, or we were already part of the lobby.
		LobbySubsystem->LobbyDetails.LobbyID = FString(Data->LobbyId);

		// Load the lobby-information.
		LobbySubsystem->LoadLobbyDetails(Data->LobbyId, [LobbySubsystem](const bool bSuccess)
		{
			if(bSuccess)
			{
				LobbySubsystem->OnJoinLobbyCompleteDelegate.Broadcast(EJoinLobbyResultCode::Success, LobbySubsystem->LobbyDetails);
				
				// TODO: Check if shadow lobby exist, if not create one.
			}
			else
			{
				// TODO: Why did this fail?
				UE_LOG(LogLobbySubsystem, Error, TEXT("Failed to load the details about this lobby."));
				LobbySubsystem->OnJoinLobbyCompleteDelegate.Broadcast(EJoinLobbyResultCode::Failure, LobbySubsystem->LobbyDetails); // Change this ELobbyResultCode::JoinFailure to false if there is a case where this may fail, then also leave the lobby.
				LobbySubsystem->LeaveLobby();
			}
		});
	}
	else
	{
		// TODO: EOS_NotFound
		UE_LOG(LogLobbySubsystem, Warning, TEXT("Failed to join lobby. ResultCode: [%s]"), *FString(EOS_EResult_ToString(Data->ResultCode)));
		LobbySubsystem->OnJoinLobbyCompleteDelegate.Broadcast(EJoinLobbyResultCode::Failure, LobbySubsystem->LobbyDetails);
	}
}


// --------------------------------------------


void ULobbySubsystem::OnLobbyUpdated(const EOS_Lobby_UpdateLobbyCallbackInfo* Data)
{
	const ULobbySubsystem* LobbySubsystem = static_cast<ULobbySubsystem*>(Data->ClientData);
	if(!LobbySubsystem) return;
	
	if(Data->ResultCode == EOS_EResult::EOS_Success)
	{
		UE_LOG(LogLobbySubsystem, Log, TEXT("Lobby updated successfully"));
	}
	else if(Data->ResultCode == EOS_EResult::EOS_TimedOut)
	{
		// Could retry here
	}
	else
		UE_LOG(LogLobbySubsystem, Warning, TEXT("Failed to update Lobby. Code: [%s]"), *FString(EOS_EResult_ToString(Data->ResultCode)));
}

/**
 * Called when a user joins/leaves/disconnects is kicked/promoted or the lobby has been closed.
 */
void ULobbySubsystem::OnLobbyMemberStatusUpdate(const EOS_Lobby_LobbyMemberStatusReceivedCallbackInfo* Data)
{
	ULobbySubsystem* LobbySubsystem = static_cast<ULobbySubsystem*>(Data->ClientData);
	const FString TargetUserID = EosProductIDToString(Data->TargetUserId);

	// Call the appropriate method based on the status.
	switch (Data->CurrentStatus)
	{
	case EOS_ELobbyMemberStatus::EOS_LMS_JOINED:
		LobbySubsystem->OnLobbyUserJoined(TargetUserID);
		break;
	case EOS_ELobbyMemberStatus::EOS_LMS_LEFT:
		LobbySubsystem->OnLobbyUserLeft(TargetUserID);
		break;
	case EOS_ELobbyMemberStatus::EOS_LMS_DISCONNECTED:
		LobbySubsystem->OnLobbyUserDisconnected(TargetUserID);
		break;
	case EOS_ELobbyMemberStatus::EOS_LMS_KICKED:
		LobbySubsystem->OnLobbyUserKicked(TargetUserID);
		break;
	case EOS_ELobbyMemberStatus::EOS_LMS_PROMOTED:
		LobbySubsystem->OnLobbyUserPromoted(TargetUserID);
		break;
	case EOS_ELobbyMemberStatus::EOS_LMS_CLOSED:
		UE_LOG(LogLobbySubsystem, Log, TEXT("The lobby has been closed and user has been removed"));
		// TODO: Update widget and clear corresponding LocalUser data, etc.
		break;
	}
}

void ULobbySubsystem::OnLobbyUserJoined(const FString& TargetUserID)
{
	UE_LOG(LogLobbySubsystem, Log, TEXT("A user has joined the lobby"));
	// TODO: Check if user is a friend. If so, get the friend user and add it to the list of connected users. This prevents api call.
	
	// Add the user to the list of users to load
	UsersToLoad.Add(TargetUserID);
	
	OnlineUserSubsystem->GetOnlineUser(TargetUserID, [this, TargetUserID](const FGetOnlineUserResult Result)
	{
		// Check if user is still in lobby after this fetch
		if(UsersToLoad.Contains(TargetUserID)) OnLobbyUserJoinedDelegate.Broadcast(Result.OnlineUser);
		else UE_LOG(LogLobbySubsystem, Log, TEXT("User left before we could load their data. OnLobbyUserJoinedDelegate will not be broadcasted."));
	});
}

void ULobbySubsystem::OnLobbyUserLeft(const FString& TargetUserID)
{
	UE_LOG(LogLobbySubsystem, Log, TEXT("A user has left the lobby"));

	LobbyDetails.RemoveMember(TargetUserID);
	OnLobbyUserLeftDelegate.Broadcast(TargetUserID); 
}

void ULobbySubsystem::OnLobbyUserDisconnected(const FString& TargetUserID)
{
	UE_LOG(LogLobbySubsystem, Log, TEXT("A user has unexpectedly left the lobby"));

	LobbyDetails.RemoveMember(TargetUserID);
	OnLobbyUserDisconnectedDelegate.Broadcast(TargetUserID);
}

void ULobbySubsystem::OnLobbyUserKicked(const FString& TargetUserID)
{
	UE_LOG(LogLobbySubsystem, Log, TEXT("A user has been kicked from the lobby"));

	LobbyDetails.RemoveMember(TargetUserID);
	OnLobbyUserKickedDelegate.Broadcast(TargetUserID);
}

void ULobbySubsystem::OnLobbyUserPromoted(const FString& TargetUserID)
{
	UE_LOG(LogLobbySubsystem, Log, TEXT("A user has been promoted to Lobby-Owner"));

	LobbyDetails.LobbyOwnerID = TargetUserID;
	OnLobbyUserPromotedDelegate.Broadcast(TargetUserID);
}


// --------------------------------------------


/**
 * Gets all the necessary information from a lobby and stores it in memory, this includes the members.
 *
 * @param LobbyID The ID of the lobby to get the details from.
 * @param OnCompleteCallback Callback to call when the function completes.
 */
void ULobbySubsystem::LoadLobbyDetails(const EOS_LobbyId LobbyID, TFunction<void(bool bSuccess)> OnCompleteCallback)
{
	const EOS_Lobby_CopyLobbyDetailsHandleOptions LobbyDetailsOptions{
		EOS_LOBBY_COPYLOBBYDETAILSHANDLE_API_LATEST,
		LobbyID,
		EosProductIDFromString(LocalUserSubsystem->GetLocalUser()->GetProductUserID())
	};
	EOS_HLobbyDetails LobbyDetailsHandle;
	EOS_EResult Result = EOS_Lobby_CopyLobbyDetailsHandle(LobbyHandle, &LobbyDetailsOptions, &LobbyDetailsHandle);
	
	if(Result == EOS_EResult::EOS_Success)
	{
		// Successfully got the LobbyDetails-Handle
		
		// Get Lobby Info
		constexpr EOS_LobbyDetails_CopyInfoOptions CopyInfoOptions{
			EOS_LOBBYDETAILS_COPYINFO_API_LATEST
		};
		EOS_LobbyDetails_Info* LobbyInfo;
		Result = EOS_LobbyDetails_CopyInfo(LobbyDetailsHandle, &CopyInfoOptions, &LobbyInfo);
		if(Result == EOS_EResult::EOS_Success)
		{
			// Successfully got the LobbyDetails

			// Store the details we need
			LobbyDetails.LobbyID = FString(LobbyInfo->LobbyId);
			LobbyDetails.LobbyOwnerID = EosProductIDToString(LobbyInfo->LobbyOwnerUserId);
			LobbyDetails.MaxMembers = LobbyInfo->MaxMembers;
			
			// Release the info to avoid memory leaks
			EOS_LobbyDetails_Info_Release(LobbyInfo);
			
			// Get the Member Count
			constexpr EOS_LobbyDetails_GetMemberCountOptions MemberCountOptions{
				EOS_LOBBYDETAILS_GETMEMBERCOUNT_API_LATEST
			};
			const uint32_t MemberCount = EOS_LobbyDetails_GetMemberCount(LobbyDetailsHandle, &MemberCountOptions);

			// Get the Members using the count, excluding the local-user.
			const FString LocalUserProductID = GetGameInstance()->GetSubsystem<ULocalUserSubsystem>()->GetLocalUser()->GetProductUserID();
			TArray<FString> MemberProductIDs;
			for(uint32_t MemberIndex = 0; MemberIndex < MemberCount; MemberIndex++)
			{
				const EOS_LobbyDetails_GetMemberByIndexOptions MemberByIndexOptions{
					EOS_LOBBYDETAILS_GETMEMBERBYINDEX_API_LATEST,
					MemberIndex
				};
				const FString MemberProductID = EosProductIDToString(EOS_LobbyDetails_GetMemberByIndex(LobbyDetailsHandle, &MemberByIndexOptions));
				if(!MemberProductID.IsEmpty() && MemberProductID != LocalUserProductID) MemberProductIDs.Add(MemberProductID);
			}
			
			// Get the user-information from all user's in the lobby
			OnlineUserSubsystem->GetOnlineUsers(MemberProductIDs, [this, OnCompleteCallback](const FGetOnlineUsersResult& Result)
			{
				if(Result.ResultCode != EGetOnlineUserResultCode::Success)
				{
					OnCompleteCallback(false);
					return;
				}

				TMap<FString, UOnlineUser*> OnlineUserMap;
				for (UOnlineUser* OnlineUser : Result.OnlineUsers)
				{
					OnlineUserMap.Add(OnlineUser->GetProductUserID(), OnlineUser);
				}

				ULocalUser* LocalUser = GetGameInstance()->GetSubsystem<ULocalUserSubsystem>()->GetLocalUser();
				OnlineUserMap.Add(LocalUser->GetProductUserID(), LocalUser);
				LobbyDetails.MemberList = OnlineUserMap;

				// Done loading data.
				OnCompleteCallback(true);
			});
		}
		else
		{
			EOS_LobbyDetails_Info_Release(LobbyInfo);
			OnCompleteCallback(false);
		}
	}
	else
	{
		OnCompleteCallback(false);
	}
}


// -------------------------------------------- Shadow Lobby -------------------------------------------- //

void ULobbySubsystem::OnCreateShadowLobbyComplete(const FShadowLobbyResult &ShadowLobbyResult)
{
	LocalPlatformLobbySubsystem->OnCreateShadowLobbyCompleteDelegate.Unbind();
	if(ShadowLobbyResult.ResultCode == EShadowLobbyResultCode::Success)
	{
		AddShadowLobbyIDAttribute(ShadowLobbyResult.LobbyDetails.LobbyID);
	}
	else UE_LOG(LogLobbySubsystem, Warning, TEXT("Failed to create Shadow-Lobby. Lobby presence and invites through platform will not work, but EOS should still work."));
}

void ULobbySubsystem::JoinShadowLobby(const uint64 ShadowLobbyId)
{
}

void ULobbySubsystem::OnJoinShadowLobbyComplete(const uint64 ShadowLobbyId)
{
}

/**
 * Creates a lobby modification where the shadow lobby's ID attribute is added.
 */
void ULobbySubsystem::AddShadowLobbyIDAttribute(const FString& ShadowLobbyID)
{
    EOS_Lobby_UpdateLobbyModificationOptions UpdateLobbyModificationOptions;
    UpdateLobbyModificationOptions.ApiVersion = EOS_LOBBY_UPDATELOBBYMODIFICATION_API_LATEST;
	UpdateLobbyModificationOptions.LocalUserId = EosProductIDFromString(LocalUserSubsystem->GetLocalUser()->GetProductUserID());
    UpdateLobbyModificationOptions.LobbyId = TCHAR_TO_ANSI(*LobbyDetails.LobbyID);
 
	// Create the lobby modification handle.
	EOS_HLobbyModification LobbyModificationHandle;
    EOS_EResult Result = EOS_Lobby_UpdateLobbyModification(LobbyHandle, &UpdateLobbyModificationOptions, &LobbyModificationHandle);
    if (Result == EOS_EResult::EOS_Success)
    {
        // Create the ShadowLobbyID attribute and set it to the shadow lobby ID.
    	EOS_Lobby_AttributeData ShadowLobbyIDAttribute;
        switch (LocalUserSubsystem->GetLocalUser()->GetPlatform())
        {
        case EPlatform::Steam:
        	ShadowLobbyIDAttribute.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
        	ShadowLobbyIDAttribute.Key = "SteamLobbyID";
        	ShadowLobbyIDAttribute.Value.AsUtf8 = TCHAR_TO_ANSI(*ShadowLobbyID);
        	ShadowLobbyIDAttribute.ValueType = EOS_ELobbyAttributeType::EOS_AT_STRING;
        	break;
        default:
        	break;
        } // TODO: Add more platforms
 
    	// Add it to the options.
        EOS_LobbyModification_AddAttributeOptions ShadowLobbyIDAttributeOptions;
        ShadowLobbyIDAttributeOptions.ApiVersion = EOS_LOBBYMODIFICATION_ADDATTRIBUTE_API_LATEST;
        ShadowLobbyIDAttributeOptions.Attribute = &ShadowLobbyIDAttribute;
        ShadowLobbyIDAttributeOptions.Visibility = EOS_ELobbyAttributeVisibility::EOS_LAT_PUBLIC;
 
    	// Add the change to the lobby modification handle.
        Result = EOS_LobbyModification_AddAttribute(LobbyModificationHandle, &ShadowLobbyIDAttributeOptions);
        if (Result == EOS_EResult::EOS_Success)
        {
            // Update the lobby with the modification handle
            EOS_Lobby_UpdateLobbyOptions UpdateLobbyOptions;
            UpdateLobbyOptions.ApiVersion = EOS_LOBBY_UPDATELOBBY_API_LATEST;
            UpdateLobbyOptions.LobbyModificationHandle = LobbyModificationHandle;
        	
            EOS_Lobby_UpdateLobby(LobbyHandle, &UpdateLobbyOptions, nullptr, [](const EOS_Lobby_UpdateLobbyCallbackInfo* Data)
            {
            	if(Data->ResultCode == EOS_EResult::EOS_Success)
            	{
					UE_LOG(LogLobbySubsystem, Log, TEXT("Shadow-Lobby-ID added to EOS-Lobby attributes."));
				}
				else UE_LOG(LogLobbySubsystem, Warning, TEXT("Failed to add Shadow-Lobby-ID to EOS-Lobby attributes. ErrorCode: [%s]"), *FString(EOS_EResult_ToString(Data->ResultCode)));
            });
        }
        else UE_LOG(LogLobbySubsystem, Warning, TEXT("Failed to add Shadow-Lobby-ID to EOS-Lobby attributes. ErrorCode: [%s]"), *FString(EOS_EResult_ToString(Result)));
    	
    	EOS_LobbyModification_Release(LobbyModificationHandle);
    }
    else UE_LOG(LogLobbySubsystem, Warning, TEXT("Failed to get Lobby-Modification-Handle. ErrorCode: [%s]"), *FString(EOS_EResult_ToString(Result)));
}
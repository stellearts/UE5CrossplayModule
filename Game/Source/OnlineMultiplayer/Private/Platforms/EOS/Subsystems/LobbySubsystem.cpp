// Copyright © 2023 Melvin Brink

#include "Platforms/EOS/Subsystems/LobbySubsystem.h"
#include "Platforms/EOS/EOSManager.h"
#include "eos_lobby.h"
#include "Platforms/Steam/Subsystems/SteamLobbySubsystem.h"
#include "UserStateSubsystem.h"
#include "Platforms/EOS/UserTypes.h"
#include "Platforms/EOS/Subsystems/ConnectSubsystem.h"
#include "Platforms/EOS/Subsystems/LocalUserSubsystem.h"
#include "Platforms/EOS/Subsystems/OnlineUserSubsystem.h"


void ULobbySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Make sure the local user state subsystem is initialized. Store it as member since I will need to use it often.
	LocalUserSubsystem = Collection.InitializeDependency<ULocalUserSubsystem>();
	
	
	// TODO: Add compatibility for other platforms. Currently only steam.
	// TODO: Maybe use preprocessor directives for platform specific code.
	SteamLobbySubsystem = Collection.InitializeDependency<USteamLobbySubsystem>();
	
	EosManager = &FEosManager::Get();
	const EOS_HPlatform PlatformHandle = EosManager->GetPlatformHandle();
	if(!PlatformHandle) return;
	LobbyHandle = EOS_Platform_GetLobbyInterface(PlatformHandle);

	
	// Register lobby member status update callback.
	EOS_Lobby_AddNotifyLobbyMemberStatusReceivedOptions Options;
	Options.ApiVersion = EOS_LOBBY_ADDNOTIFYLOBBYMEMBERSTATUSRECEIVED_API_LATEST;
	OnLobbyMemberStatusUpdateID = EOS_Lobby_AddNotifyLobbyMemberStatusReceived(LobbyHandle, &Options, this, &ULobbySubsystem::OnLobbyMemberStatusUpdate);
}


// --------------------------------------------


void ULobbySubsystem::CreateLobby()
{
	// we should not be able to create a lobby if we are already in one, user should leave the current lobby first.
	if(LocalUser->IsInLobby())
	{
		UE_LOG(LogLobbySubsystem, Warning, TEXT("Cannot create a lobby while already in one."));
		return;
	}
	
	EOS_Lobby_CreateLobbyOptions CreateLobbyOptions;
	CreateLobbyOptions.ApiVersion = EOS_LOBBY_CREATELOBBY_API_LATEST;
	CreateLobbyOptions.LocalUserId = LocalUser->GetProductUserID();
	CreateLobbyOptions.MaxLobbyMembers = 4; // TODO: Change depending on the game-mode with most players. Lock lobby when in-game to prevent people from joining.
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
	
	EOS_Lobby_CreateLobby(LobbyHandle, &CreateLobbyOptions, this, &ULobbySubsystem::OnCreateLobbyComplete);
}

void ULobbySubsystem::OnCreateLobbyComplete(const EOS_Lobby_CreateLobbyCallbackInfo* Data)
{
	ULobbySubsystem* LobbySubsystem = static_cast<ULobbySubsystem*>(Data->ClientData);
	if(!LobbySubsystem) return;
	ULocalUser* LocalUser = LobbySubsystem->LocalUser;
	if(!LocalUser) return;
	
	if(Data->ResultCode == EOS_EResult::EOS_Success)
	{
		UE_LOG(LogLobbySubsystem, Log, TEXT("Lobby created successfully"));
		
		LocalUser->SetLobbyID(Data->LobbyId);
		LobbySubsystem->OnCreateLobbyCompleteDelegate.Broadcast(true);

		// TODO: Add compatibility for other platforms. Currently only steam.
		// Create a shadow lobby for platforms besides Epic to show the lobby for other players from the same platform.
		if(LocalUser->GetPlatform() == EOS_EExternalAccountType::EOS_EAT_STEAM)
		{
			// Create a shadow lobby on steam. This will let Steam users join using the overlay.
			LobbySubsystem->CreateShadowLobby();
		}
	}
	else if(Data->ResultCode == EOS_EResult::EOS_Lobby_PresenceLobbyExists)
	{
		// If for some reason the lobby already exists, due to a crash or something, we can just join it.
		UE_LOG(LogLobbySubsystem, Log, TEXT("Lobby already exists, joining instead..."));	
		LobbySubsystem->JoinLobbyByUserID(LocalUser->GetProductUserID());
	}
	else
	{
		UE_LOG(LogLobbySubsystem, Warning, TEXT("Failed to create Lobby. Code: [%s]"), *FString(EOS_EResult_ToString(Data->ResultCode)));
		LobbySubsystem->OnCreateLobbyCompleteDelegate.Broadcast(false);
	}
}


// --------------------------------------------


void ULobbySubsystem::JoinLobbyByID(const EOS_LobbyId LobbyID)
{
	// Create the lobby search handle.
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
	SetLobbyIdOptions.LobbyId = LobbyID;
	EOS_LobbySearch_SetLobbyId(LobbySearchByLobbyIDHandle, &SetLobbyIdOptions);

	// TODO: Continue
}

void ULobbySubsystem::JoinLobbyByUserID(const EOS_ProductUserId UserID)
{
	// Create the lobby search handle.
	EOS_Lobby_CreateLobbySearchOptions LobbySearchOptions;
	LobbySearchOptions.ApiVersion = EOS_LOBBY_CREATELOBBYSEARCH_API_LATEST;
	LobbySearchOptions.MaxResults = 1;
	
	if(EOS_Lobby_CreateLobbySearch(LobbyHandle, &LobbySearchOptions, &LobbySearchByUserIDHandle) != EOS_EResult::EOS_Success)
	{
		UE_LOG(LogLobbySubsystem, Error, TEXT("Failed to create lobby search handle."));
		OnJoinLobbyCompleteDelegate.Broadcast(false);
		return;
	}

	// Set the user id to search for, which is the local user id.
	EOS_LobbySearch_SetTargetUserIdOptions SetTargetUserIdOptions;
	SetTargetUserIdOptions.ApiVersion = EOS_LOBBYSEARCH_SETTARGETUSERID_API_LATEST;
	SetTargetUserIdOptions.TargetUserId = UserID;
	EOS_LobbySearch_SetTargetUserId(LobbySearchByUserIDHandle, &SetTargetUserIdOptions);
	
	EOS_LobbySearch_FindOptions LobbySearchFindOptions;
	LobbySearchFindOptions.ApiVersion = EOS_LOBBYSEARCH_FIND_API_LATEST;
	LobbySearchFindOptions.LocalUserId = LocalUser->GetProductUserID();

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
				EOS_Lobby_JoinLobbyOptions JoinOptions;
				JoinOptions.ApiVersion = EOS_LOBBY_JOINLOBBY_API_LATEST;
				JoinOptions.LocalUserId = LobbySubsystem->LocalUser->GetProductUserID();
				JoinOptions.LobbyDetailsHandle = LobbyDetailsHandle;
			    
				EOS_Lobby_JoinLobby(LobbySubsystem->LobbyHandle, &JoinOptions, LobbySubsystem, &ULobbySubsystem::OnJoinLobbyComplete);

				// Release the handle after using it
				EOS_LobbyDetails_Release(LobbyDetailsHandle);
			}
			else
			{
				UE_LOG(LogLobbySubsystem, Warning, TEXT("EOS_LobbySearch_CopySearchResultByIndex Failed with Result Code: [%s]"), *FString(EOS_EResult_ToString(SearchResultCode)));
				LobbySubsystem->OnJoinLobbyCompleteDelegate.Broadcast(false);
			}
		}
		else
		{
			UE_LOG(LogLobbySubsystem, Warning, TEXT("Failed to find a lobby with Result Code: [%s]"), *FString(EOS_EResult_ToString(Data->ResultCode)));
			LobbySubsystem->OnJoinLobbyCompleteDelegate.Broadcast(false);
		}
	});
}

void ULobbySubsystem::JoinLobbyByHandle(const EOS_HLobbyDetails LobbyDetailsHandle)
{
	if(!LobbyHandle) return;
	
	EOS_Lobby_JoinLobbyOptions JoinOptions;
	JoinOptions.ApiVersion = EOS_LOBBY_JOINLOBBY_API_LATEST;
	JoinOptions.LocalUserId = LocalUser->GetProductUserID();
	JoinOptions.LobbyDetailsHandle = LobbyDetailsHandle;
    
	EOS_Lobby_JoinLobby(LobbyHandle, &JoinOptions, this, &ULobbySubsystem::OnJoinLobbyComplete);
	
	// Release the handle after using it
	EOS_LobbyDetails_Release(LobbyDetailsHandle);
}

void ULobbySubsystem::OnJoinLobbyComplete(const EOS_Lobby_JoinLobbyCallbackInfo* Data)
{
	// TODO: if from steam, join the shadow lobby. If steam lobby does not exist, create one.
	// TODO: Also check if the steam lobby you try to join exist at all, maybe it was deleted because the only member left crashed and could not clear the attribute on the eos lobby.

	const ULobbySubsystem* LobbySubsystem = static_cast<ULobbySubsystem*>(Data->ClientData);
	ULocalUser* LocalUser = LobbySubsystem->LocalUser;
	
	if(Data->ResultCode == EOS_EResult::EOS_Success)
	{
		UE_LOG(LogLobbySubsystem, Log, TEXT("Successfully joined lobby"));
		
		LocalUser->SetLobbyID(Data->LobbyId);
		LobbySubsystem->OnJoinLobbyCompleteDelegate.Broadcast(true);

		// TODO: Check if shadow lobby exist, if not create one.
	}else if(Data->ResultCode == EOS_EResult::EOS_Lobby_LobbyAlreadyExists)
	{
		// I guess this means we are already in the lobby.
		UE_LOG(LogLobbySubsystem, Log, TEXT("Already in lobby."));
		
		LocalUser->SetLobbyID(Data->LobbyId);
		LobbySubsystem->OnJoinLobbyCompleteDelegate.Broadcast(true);
		
		// TODO: Check if shadow lobby exist, if not create one.
	}
	else
	{
		UE_LOG(LogLobbySubsystem, Warning, TEXT("Failed to join lobby"));
		LobbySubsystem->OnJoinLobbyCompleteDelegate.Broadcast(false);
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

void ULobbySubsystem::OnLobbyMemberStatusUpdate(const EOS_Lobby_LobbyMemberStatusReceivedCallbackInfo* Data)
{
	ULobbySubsystem* LobbySubsystem = static_cast<ULobbySubsystem*>(Data->ClientData);

	// Call the appropriate method based on the status.
	switch (Data->CurrentStatus)
	{
	case EOS_ELobbyMemberStatus::EOS_LMS_JOINED:
		LobbySubsystem->OnLobbyUserJoined(Data->TargetUserId);
		break;
	case EOS_ELobbyMemberStatus::EOS_LMS_LEFT:
		LobbySubsystem->OnLobbyUserLeft(Data->TargetUserId);
		break;
	case EOS_ELobbyMemberStatus::EOS_LMS_DISCONNECTED:
		LobbySubsystem->OnLobbyUserDisconnected(Data->TargetUserId);
		break;
	case EOS_ELobbyMemberStatus::EOS_LMS_KICKED:
		LobbySubsystem->OnLobbyUserKicked(Data->TargetUserId);
		break;
	case EOS_ELobbyMemberStatus::EOS_LMS_PROMOTED:
		LobbySubsystem->OnLobbyUserPromoted(Data->TargetUserId);
		break;
	case EOS_ELobbyMemberStatus::EOS_LMS_CLOSED:
		UE_LOG(LogLobbySubsystem, Log, TEXT("The lobby has been closed and user has been removed"));
		// TODO: Update widget and clear corresponding LocalUserState data, etc.
		break;
	}
}

void ULobbySubsystem::OnLobbyUserJoined(const EOS_ProductUserId TargetUserID)
{
	UE_LOG(LogLobbySubsystem, Log, TEXT("A user has joined the lobby"));
	// TODO: Check if user is a friend. If so, get the friend user and add it to the list of connected users. This prevents api call.


	

	// Add the user to the list of users to load.a
	UsersToLoad.Add(TargetUserID);
	
	// Get the user info and store it in the connected users list.
	UConnectSubsystem* ConnectSubsystem = GetGameInstance()->GetSubsystem<UConnectSubsystem>();
	TArray<EOS_ProductUserId> UserIDs = {TargetUserID};
	ConnectSubsystem->GetUserInfo(UserIDs, [this, TargetUserID](FUsersMap OnlineUsersMapResult)
	{
		UUser* OnlineUser = nullptr;
		if(OnlineUsersMapResult.IsEmpty() || !OnlineUsersMapResult[0])
		{
			OnlineUser = NewObject<UUser>();
			UE_LOG(LogLobbySubsystem, Error, TEXT("Failed to get user info for user that joined the lobby"));
			OnlineUser->SetDisplayName("Unknown");
			OnlineUser->SetProductUserID(TargetUserID);
		}
		else OnlineUser = OnlineUsersMapResult[0]; // Get first user in the map since we only requested one for user.

		UOnlineUserSubsystem* OnlineUserSubsystem = GetGameInstance()->GetSubsystem<UOnlineUserSubsystem>();
		OnlineUserSubsystem->AddUser(OnlineUser, Lobby);

		if(UsersToLoad.Find(TargetUserID) != INDEX_NONE)
		{
			OnLobbyUserJoinedDelegate.Broadcast(OnlineUser);
		}
		else UE_LOG(LogLobbySubsystem, Warning, TEXT("User left before we could load their data. Delegate will not be broadcasted."));
	});



	
	// OnLobbyUserJoinedDelegate.Broadcast();
}

void ULobbySubsystem::OnLobbyUserLeft(const EOS_ProductUserId TargetUserID)
{
	UE_LOG(LogLobbySubsystem, Log, TEXT("A user has left the lobby"));
	OnLobbyUserLeftDelegate.Broadcast();
}

void ULobbySubsystem::OnLobbyUserDisconnected(const EOS_ProductUserId TargetUserID)
{
	UE_LOG(LogLobbySubsystem, Log, TEXT("A user has unexpectedly left the lobby"));
	OnLobbyUserDisconnectedDelegate.Broadcast();
}

void ULobbySubsystem::OnLobbyUserKicked(const EOS_ProductUserId TargetUserID)
{
	UE_LOG(LogLobbySubsystem, Log, TEXT("A user has been kicked from the lobby"));
	OnLobbyUserKickedDelegate.Broadcast();
}

void ULobbySubsystem::OnLobbyUserPromoted(const EOS_ProductUserId TargetUserID)
{
	UE_LOG(LogLobbySubsystem, Log, TEXT("A user has been promoted to lobby owner"));
	OnLobbyUserPromotedDelegate.Broadcast();
}


// -------------------------------------------- Shadow Lobby -------------------------------------------- //


void ULobbySubsystem::CreateShadowLobby()
{
	OnCreateShadowLobbyCompleteDelegateHandle = SteamLobbySubsystem->OnCreateShadowLobbyCompleteDelegate.AddUObject(this, &ULobbySubsystem::OnCreateShadowLobbyComplete);
	SteamLobbySubsystem->CreateShadowLobby();
}

/**
 * Result from creating a shadow lobby.
 *
 * Already contains the EOS-Lobby-ID data in this shadow lobby on success, to redirect others to this EOS lobby.
 *
 * @param ShadowLobbyID The ID of the shadow lobby, 0 if failed to create.
 */
void ULobbySubsystem::OnCreateShadowLobbyComplete(const uint64 ShadowLobbyID)
{
	SteamLobbySubsystem->OnCreateShadowLobbyCompleteDelegate.Remove(OnCreateShadowLobbyCompleteDelegateHandle);
	LocalUser->SetShadowLobbyID(ShadowLobbyID); // 0 if failed to create shadow lobby.

	if(ShadowLobbyID)
	{
		UE_LOG(LogLobbySubsystem, Log, TEXT("Shadow-Lobby created."));
		AddShadowLobbyIdAttribute(ShadowLobbyID);
	}
	else
	{
		UE_LOG(LogLobbySubsystem, Warning, TEXT("Failed to create Shadow-Lobby. Lobby presence and invites through platform will not work. EOS should still work."));
		// TODO: Destroy or leave this shadow lobby.
	}
}

void ULobbySubsystem::JoinShadowLobby(const uint64 ShadowLobbyId)
{
}

void ULobbySubsystem::OnJoinShadowLobbyComplete(const uint64 ShadowLobbyId)
{
}


// --------------------------------------------


/**
 * Creates a lobby modification where the SteamInviterId attribute is added.
 */
void ULobbySubsystem::AddShadowLobbyIdAttribute(const uint64 ShadowLobbyID)
{
	// TODO: Add multiple platforms compatibility.
	
    EOS_Lobby_UpdateLobbyModificationOptions UpdateLobbyModificationOptions;
    UpdateLobbyModificationOptions.ApiVersion = EOS_LOBBY_UPDATELOBBYMODIFICATION_API_LATEST;
	UpdateLobbyModificationOptions.LocalUserId = LocalUser->GetProductUserID();
    UpdateLobbyModificationOptions.LobbyId = LocalUser->GetLobbyID().c_str();

	// Create the lobby modification handle.
	EOS_HLobbyModification LobbyModificationHandle;
    EOS_EResult Result = EOS_Lobby_UpdateLobbyModification(LobbyHandle, &UpdateLobbyModificationOptions, &LobbyModificationHandle);
    if (Result == EOS_EResult::EOS_Success)
    {
        // Create the SteamLobbyID attribute and set it to the shadow lobby ID.
        EOS_Lobby_AttributeData SteamLobbyIDAttribute;
        SteamLobbyIDAttribute.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
        SteamLobbyIDAttribute.Key = "SteamLobbyID"; // TODO: Add multiple platforms compatibility.
        SteamLobbyIDAttribute.Value.AsInt64 = ShadowLobbyID;
        SteamLobbyIDAttribute.ValueType = EOS_ELobbyAttributeType::EOS_AT_INT64;

    	// Add it to the options.
        EOS_LobbyModification_AddAttributeOptions SteamLobbyIDAttributeOptions;
        SteamLobbyIDAttributeOptions.ApiVersion = EOS_LOBBYMODIFICATION_ADDATTRIBUTE_API_LATEST;
        SteamLobbyIDAttributeOptions.Attribute = &SteamLobbyIDAttribute;
        SteamLobbyIDAttributeOptions.Visibility = EOS_ELobbyAttributeVisibility::EOS_LAT_PUBLIC;

    	// Add the change to the lobby modification handle.
        Result = EOS_LobbyModification_AddAttribute(LobbyModificationHandle, &SteamLobbyIDAttributeOptions);
        if (Result == EOS_EResult::EOS_Success)
        {
            // Update the lobby with the changed modification handle.
            EOS_Lobby_UpdateLobbyOptions UpdateLobbyOptions;
            UpdateLobbyOptions.ApiVersion = EOS_LOBBY_UPDATELOBBY_API_LATEST;
            UpdateLobbyOptions.LobbyModificationHandle = LobbyModificationHandle;
            EOS_Lobby_UpdateLobby(LobbyHandle, &UpdateLobbyOptions, nullptr, &ULobbySubsystem::OnAddShadowLobbyIdAttributeComplete);
        }
        else
            UE_LOG(LogLobbySubsystem, Warning, TEXT("Failed to add Shadow-Lobby-ID attribute. Code: [%s]"), *FString(EOS_EResult_ToString(Result)));

    	// Release the lobby modification handle.
    	EOS_LobbyModification_Release(LobbyModificationHandle);
    }
    else
        UE_LOG(LogLobbySubsystem, Warning, TEXT("Failed to get lobby modification handle. Code: [%s]"), *FString(EOS_EResult_ToString(Result)));
}

/**
 * Callback for when the lobby has tried to add the ShadowLobbyID attribute.
 */
void ULobbySubsystem::OnAddShadowLobbyIdAttributeComplete(const EOS_Lobby_UpdateLobbyCallbackInfo* Data)
{
	if(Data->ResultCode == EOS_EResult::EOS_Success)
	{
		UE_LOG(LogLobbySubsystem, Log, TEXT("Shadow-Lobby-ID attribute added."));
	}
	else
	{
		UE_LOG(LogLobbySubsystem, Warning, TEXT("Failed to add Shadow-Lobby-ID attribute. Code: [%s]"), *FString(EOS_EResult_ToString(Data->ResultCode)));
		// TODO: Retry or delete the shadow lobby. Should not reach here, but just in case.
		// A second steam user can try to create the shadow lobby.
	}
}
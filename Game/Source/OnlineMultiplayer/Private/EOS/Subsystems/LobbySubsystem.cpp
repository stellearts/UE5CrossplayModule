// Copyright © 2023 Melvin Brink

#include "EOS/Subsystems/LobbySubsystem.h"

#include "EOS/EOSManager.h"
#include "eos_lobby.h"
#include "EOS/Subsystems/LocalUserStateSubsystem.h"
#include "Steam/SteamLobbyManager.h"



void ULobbySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Make sure the local user state subsystem is initialized. Store it as member since we will need to use it often.
	LocalUserState = Collection.InitializeDependency<ULocalUserStateSubsystem>();
	
	// TODO: Add compatibility for other platforms. Currently only steam.
	// TODO: Maybe use preprocessor directives for platform specific code.
	SteamLobbyManager = MakeUnique<FSteamLobbyManager>(GetGameInstance());
	
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
	if(!LocalUserState) return;
	
	// we should not be able to create a lobby if we are already in one, user should leave the current lobby first.
	if(LocalUserState->IsInLobby())
	{
		UE_LOG(LogLobbySubsystem, Warning, TEXT("Cannot create a lobby while already in one."));
		return;
	}
	
	EOS_Lobby_CreateLobbyOptions CreateLobbyOptions;
	CreateLobbyOptions.ApiVersion = EOS_LOBBY_CREATELOBBY_API_LATEST;
	CreateLobbyOptions.LocalUserId = LocalUserState->GetProductUserID();
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
	ULocalUserStateSubsystem* LocalUserState = LobbySubsystem->LocalUserState;
	if(!LocalUserState) return;
	
	if(Data->ResultCode == EOS_EResult::EOS_Success)
	{
		UE_LOG(LogLobbySubsystem, Log, TEXT("Lobby created successfully"));
		
		LocalUserState->SetLobbyID(Data->LobbyId);
		LobbySubsystem->OnCreateLobbyCompleteDelegate.Broadcast(true);

		// TODO: Add compatibility for other platforms. Currently only steam.
		// Create a shadow lobby for platforms besides Epic to show the lobby for other players from the same platform.
		if(LocalUserState->GetPlatformType() == EPlatformType::PlatformType_Steam)
		{
			// Create a shadow lobby on steam. This will let Steam users join using the overlay.
			LobbySubsystem->CreateShadowLobby();
		}
	}
	else if(Data->ResultCode == EOS_EResult::EOS_Lobby_PresenceLobbyExists)
	{
		// If for some reason the lobby already exists, due to a crash or something, we can just join it.
		UE_LOG(LogLobbySubsystem, Log, TEXT("Lobby already exists, joining instead..."));	
		LobbySubsystem->JoinLobbyByUserID(LocalUserState->GetProductUserID());
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
	if(!LocalUserState) return;
	
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
	if(!LocalUserState) return;

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
	LobbySearchFindOptions.LocalUserId = LocalUserState->GetProductUserID();

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
				JoinOptions.LocalUserId = LobbySubsystem->LocalUserState->GetProductUserID();
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
	if(!LobbyHandle || !LocalUserState) return;
	
	EOS_Lobby_JoinLobbyOptions JoinOptions;
	JoinOptions.ApiVersion = EOS_LOBBY_JOINLOBBY_API_LATEST;
	JoinOptions.LocalUserId = LocalUserState->GetProductUserID();
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
	ULocalUserStateSubsystem* LocalUserState = LobbySubsystem->LocalUserState;
	
	if(Data->ResultCode == EOS_EResult::EOS_Success)
	{
		UE_LOG(LogLobbySubsystem, Log, TEXT("Successfully joined lobby"));
		
		LocalUserState->SetLobbyID(Data->LobbyId);
		LobbySubsystem->OnJoinLobbyCompleteDelegate.Broadcast(true);

		// TODO: Check if shadow lobby exist, if not create one.
	}else if(Data->ResultCode == EOS_EResult::EOS_Lobby_LobbyAlreadyExists)
	{
		// I guess this means we are already in the lobby.
		UE_LOG(LogLobbySubsystem, Log, TEXT("Already in lobby."));
		
		LocalUserState->SetLobbyID(Data->LobbyId);
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
	const ULobbySubsystem* LobbySubsystem = static_cast<ULobbySubsystem*>(Data->ClientData);
	switch (Data->CurrentStatus)
	{
	case EOS_ELobbyMemberStatus::EOS_LMS_JOINED:
		UE_LOG(LogLobbySubsystem, Log, TEXT("A user has joined the lobby"));
		LobbySubsystem->OnLobbyUserJoinedDelegate.Broadcast();
		break;
	case EOS_ELobbyMemberStatus::EOS_LMS_LEFT:
		UE_LOG(LogLobbySubsystem, Log, TEXT("A user has left the lobby"));
		LobbySubsystem->OnLobbyUserLeftDelegate.Broadcast();
		break;
	case EOS_ELobbyMemberStatus::EOS_LMS_DISCONNECTED:
		UE_LOG(LogLobbySubsystem, Log, TEXT("A user has unexpectedly left the lobby"));
		LobbySubsystem->OnLobbyUserDisconnectedDelegate.Broadcast();
		break;
	case EOS_ELobbyMemberStatus::EOS_LMS_KICKED:
		UE_LOG(LogLobbySubsystem, Log, TEXT("A user has been kicked from the lobby"));
		LobbySubsystem->OnLobbyUserKickedDelegate.Broadcast();
		break;
	case EOS_ELobbyMemberStatus::EOS_LMS_PROMOTED:
		UE_LOG(LogLobbySubsystem, Log, TEXT("A user has been promoted to lobby owner"));
		LobbySubsystem->OnLobbyUserPromotedDelegate.Broadcast();
		break;
	case EOS_ELobbyMemberStatus::EOS_LMS_CLOSED:
		UE_LOG(LogLobbySubsystem, Log, TEXT("The lobby has been closed and user has been removed"));
		// TODO: Update widget and clear corresponding LocalUserState data, etc.
		break;
	}
}


// -------------------------------------------- Shadow Lobby -------------------------------------------- //


void ULobbySubsystem::CreateShadowLobby()
{
	if(!LocalUserState || !SteamLobbyManager.IsValid()) return;
	
	OnCreateShadowLobbyCompleteDelegateHandle = SteamLobbyManager->OnCreateShadowLobbyCompleteDelegate.AddUObject(this, &ULobbySubsystem::OnCreateShadowLobbyComplete);
	SteamLobbyManager->CreateShadowLobby();
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
	if(!LocalUserState || !SteamLobbyManager.IsValid()) return;
	
	SteamLobbyManager->OnCreateShadowLobbyCompleteDelegate.Remove(OnCreateShadowLobbyCompleteDelegateHandle);
	LocalUserState->SetShadowLobbyID(ShadowLobbyID); // 0 if failed to create shadow lobby.

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
	if(!LocalUserState) return;
	
    EOS_Lobby_UpdateLobbyModificationOptions UpdateLobbyModificationOptions;
    UpdateLobbyModificationOptions.ApiVersion = EOS_LOBBY_UPDATELOBBYMODIFICATION_API_LATEST;
	UpdateLobbyModificationOptions.LocalUserId = LocalUserState->GetProductUserID();
    UpdateLobbyModificationOptions.LobbyId = LocalUserState->GetLobbyID();

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
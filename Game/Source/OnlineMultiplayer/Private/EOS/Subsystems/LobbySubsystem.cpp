// Copyright © 2023 Melvin Brink

#include "EOS/Subsystems/LobbySubsystem.h"

#include "EOS/EOSManager.h"
#include "eos_lobby.h"
#include "Steam/SteamLobbyManager.h"



void ULobbySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	EosManager = &FEosManager::Get();
	LocalUserState = EosManager->GetLocalUserState();

	if(LocalUserState->GetPlatformType() == EPlatformType::PlatformType_Steam)
	{
		SteamLobbyManager = MakeUnique<FSteamLobbyManager>();
	}
	
	const EOS_HPlatform PlatformHandle = EosManager->GetPlatformHandle();
	if(!PlatformHandle) return;
	LobbyHandle = EOS_Platform_GetLobbyInterface(PlatformHandle);

	// Create the lobby search handle.
	// Used to find our own lobby, for if we crashed or some other reason, and need to rejoin.
	EOS_Lobby_CreateLobbySearchOptions LobbySearchOptions;
	LobbySearchOptions.ApiVersion = EOS_LOBBY_CREATELOBBYSEARCH_API_LATEST;
	LobbySearchOptions.MaxResults = 1;
	EOS_Lobby_CreateLobbySearch(LobbyHandle, &LobbySearchOptions, &LobbySearchHandle);
}


// --------------------------------------------


void ULobbySubsystem::CreateLobby()
{
	// we should not be able to create a lobby if we are already in one, user should leave the current lobby first.
	
	if(LocalUserState->GetLobbyID())
	{
		UE_LOG(LogLobbySubsystem, Warning, TEXT("Cannot create a lobby while already in one."));
		return;
	}
	
	EOS_Lobby_CreateLobbyOptions CreateLobbyOptions;
	CreateLobbyOptions.ApiVersion = EOS_LOBBY_CREATELOBBY_API_LATEST;
	CreateLobbyOptions.LocalUserId = LocalUserState->GetProductUserId();
	CreateLobbyOptions.MaxLobbyMembers = 4;
	CreateLobbyOptions.PermissionLevel = EOS_ELobbyPermissionLevel::EOS_LPL_INVITEONLY;
	CreateLobbyOptions.bPresenceEnabled = true;
	CreateLobbyOptions.bAllowInvites = false;
	CreateLobbyOptions.BucketId = "PresenceLobby"; // TODO: What is this?
	CreateLobbyOptions.bDisableHostMigration = false;
	CreateLobbyOptions.bEnableRTCRoom = false;
	CreateLobbyOptions.LocalRTCOptions = nullptr;
	CreateLobbyOptions.LobbyId = nullptr;
	CreateLobbyOptions.bEnableJoinById = true;
	CreateLobbyOptions.bRejoinAfterKickRequiresInvite = false;
	
	EOS_Lobby_CreateLobby(LobbyHandle, &CreateLobbyOptions, this, &ULobbySubsystem::OnCreateLobbyComplete);
}

void ULobbySubsystem::OnCreateLobbyComplete(const EOS_Lobby_CreateLobbyCallbackInfo* Data)
{
	ULobbySubsystem* LobbySubsystem = static_cast<ULobbySubsystem*>(Data->ClientData);
	if(!LobbySubsystem) return;
	
	if(Data->ResultCode == EOS_EResult::EOS_Success)
	{
		UE_LOG(LogLobbySubsystem, Log, TEXT("Lobby created successfully"));
		LobbySubsystem->LocalUserState->SetLobbyID(Data->LobbyId);

		// TODO: Add compatibility for other platforms. Currently only steam.
		// Create a shadow lobby for platforms besides Epic to show the lobby for other players from the same platform.
		if(LobbySubsystem->LocalUserState->GetPlatformType() == EPlatformType::PlatformType_Steam && LobbySubsystem->SteamLobbyManager.IsValid())
		{
			// Create a shadow lobby on steam. This will let Steam users join using the overlay.
			LobbySubsystem->CreateShadowLobbyCompleteDelegateHandle =
				LobbySubsystem->SteamLobbyManager->OnCreateShadowLobbyCompleteDelegate.AddUObject(LobbySubsystem, &ULobbySubsystem::OnCreateShadowLobbyComplete);
			LobbySubsystem->SteamLobbyManager->CreateShadowLobby(Data->LobbyId);
		}
	}
	else if(Data->ResultCode == EOS_EResult::EOS_Lobby_PresenceLobbyExists)
	{
		// If for some reason the lobby already exists, due to a crash or something, we can just join it.

		// Set the user id to search for, which is the local user id.
		EOS_LobbySearch_SetTargetUserIdOptions SetTargetUserIdOptions;
		SetTargetUserIdOptions.ApiVersion = EOS_LOBBYSEARCH_SETTARGETUSERID_API_LATEST;
		SetTargetUserIdOptions.TargetUserId = LobbySubsystem->LocalUserState->GetProductUserId();
		EOS_LobbySearch_SetTargetUserId(LobbySubsystem->LobbySearchHandle, &SetTargetUserIdOptions);

		// Create the find options.
		EOS_LobbySearch_FindOptions LobbySearchFindOptions;
		LobbySearchFindOptions.ApiVersion = EOS_LOBBYSEARCH_FIND_API_LATEST;
		LobbySearchFindOptions.LocalUserId = LobbySubsystem->LocalUserState->GetProductUserId();

		// &ULobbySubsystem::OnFindOurLobbyComplete;
		// EOS_LobbySearch_Find(LobbySubsystem->LobbySearchHandle, &LobbySearchFindOptions, LobbySubsystem, &ULobbySubsystem::OnLobbySearchComplete);
	}
	else
		UE_LOG(LogLobbySubsystem, Warning, TEXT("Failed to create Lobby. Code: [%s]"), *FString(EOS_EResult_ToString(Data->ResultCode)));
}


// --------------------------------------------


void ULobbySubsystem::JoinLobby(const EOS_HLobbyDetails LobbyDetailsHandle)
{
	EOS_Lobby_JoinLobbyOptions JoinOptions;
	JoinOptions.ApiVersion = EOS_LOBBY_JOINLOBBY_API_LATEST;
	JoinOptions.LocalUserId = LocalUserState->GetProductUserId();
	JoinOptions.LobbyDetailsHandle = LobbyDetailsHandle;
    
	EOS_Lobby_JoinLobby(LobbyHandle, &JoinOptions, this, &ULobbySubsystem::OnJoinLobbyComplete);
}

void ULobbySubsystem::OnJoinLobbyComplete(const EOS_Lobby_JoinLobbyCallbackInfo* Data)
{
	if(Data->ResultCode == EOS_EResult::EOS_Success)
	{
		// Successfully joined the lobby.
		UE_LOG(LogLobbySubsystem, Warning, TEXT("Successfully joined lobby"));
	}
	else
		UE_LOG(LogLobbySubsystem, Warning, TEXT("Failed to join lobby"));
}


// --------------------------------------------


void ULobbySubsystem::OnLobbySearchComplete(const EOS_LobbySearch_FindCallbackInfo* Data)
{
	if(Data->ResultCode == EOS_EResult::EOS_Success)
	{
		ULobbySubsystem* LobbySubsystem = static_cast<ULobbySubsystem*>(Data->ClientData);

		// Create the search results options and set it to find the first lobby found, should only be one.
		EOS_LobbySearch_CopySearchResultByIndexOptions Options;
		Options.ApiVersion = EOS_LOBBYSEARCH_COPYSEARCHRESULTBYINDEX_API_LATEST;
		Options.LobbyIndex = 0;
		
		EOS_HLobbyDetails LobbyDetailsHandle;
		const EOS_EResult ResultCode = EOS_LobbySearch_CopySearchResultByIndex(LobbySubsystem->LobbySearchHandle, &Options, &LobbyDetailsHandle);
		if(ResultCode == EOS_EResult::EOS_Success)
		{
			LobbySubsystem->JoinLobby(LobbyDetailsHandle);
		}
		else
			UE_LOG(LogLobbySubsystem, Warning, TEXT("Failed to get lobby details handle. Code: [%s]"), *FString(EOS_EResult_ToString(ResultCode)));
	}
	else
		UE_LOG(LogLobbySubsystem, Warning, TEXT("Failed to find a lobby"));
}


// --------------------------------------------


void ULobbySubsystem::OnLobbyUpdated(const EOS_Lobby_UpdateLobbyCallbackInfo* Data)
{
	const ULobbySubsystem* LobbySubsystem = static_cast<ULobbySubsystem*>(Data->ClientData);
	if(!LobbySubsystem) return;
	
	if(Data->ResultCode == EOS_EResult::EOS_Success)
	{
		UE_LOG(LogLobbySubsystem, Warning, TEXT("Lobby updated successfully"));
	}
	else if(Data->ResultCode == EOS_EResult::EOS_TimedOut)
	{
		// Could retry here
	}
	else
		UE_LOG(LogLobbySubsystem, Warning, TEXT("Failed to update Lobby. Code: [%s]"), *FString(EOS_EResult_ToString(Data->ResultCode)));
}


// --------------------------------------------


void ULobbySubsystem::OnCreateShadowLobbyComplete(const uint64 ShadowLobbyId)
{
	if(!SteamLobbyManager) return;
	SteamLobbyManager->OnCreateShadowLobbyCompleteDelegate.Remove(CreateShadowLobbyCompleteDelegateHandle);
	LocalUserState->SetShadowLobbyID(ShadowLobbyId); // Will be 0 if failed to create shadow lobby.

	if(ShadowLobbyId)
	{
		UE_LOG(LogLobbySubsystem, Log, TEXT("Shadow lobby created."));
	}
	else
	{
		UE_LOG(LogLobbySubsystem, Error, TEXT("Failed to create shadow lobby. Lobby presence and invites through platform may not work."));
	}
}


// --------------------------------------------


/**
 * Creates a lobby modification where the SteamInviterId attribute is added.
 */
void ULobbySubsystem::ApplySteamInviterAttributeModification(const EOS_LobbyId LobbyId)
{
	// Get the lobby modification handle
    EOS_HLobbyModification LobbyModificationHandle = nullptr;
    EOS_Lobby_UpdateLobbyModificationOptions UpdateLobbyModificationOptions;
    UpdateLobbyModificationOptions.ApiVersion = EOS_LOBBY_UPDATELOBBYMODIFICATION_API_LATEST;
	UpdateLobbyModificationOptions.LocalUserId = LocalUserState->GetProductUserId();
    UpdateLobbyModificationOptions.LobbyId = LobbyId;
    
    EOS_EResult Result = EOS_Lobby_UpdateLobbyModification(LobbyHandle, &UpdateLobbyModificationOptions, &LobbyModificationHandle);
    if (Result == EOS_EResult::EOS_Success)
    {
        // Add the SteamInviterId attribute
        EOS_Lobby_AttributeData SteamInviterIdAttribute;
        SteamInviterIdAttribute.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
        SteamInviterIdAttribute.Key = "SteamInviterID";
        SteamInviterIdAttribute.Value.AsInt64 = EosManager->GetSteamID().ConvertToUint64();
        SteamInviterIdAttribute.ValueType = EOS_ELobbyAttributeType::EOS_AT_INT64;

        EOS_LobbyModification_AddAttributeOptions AddSteamInviterIdOptions;
        AddSteamInviterIdOptions.ApiVersion = EOS_LOBBYMODIFICATION_ADDATTRIBUTE_API_LATEST;
        AddSteamInviterIdOptions.Attribute = &SteamInviterIdAttribute;
        AddSteamInviterIdOptions.Visibility = EOS_ELobbyAttributeVisibility::EOS_LAT_PUBLIC;

        Result = EOS_LobbyModification_AddAttribute(LobbyModificationHandle, &AddSteamInviterIdOptions);
        if (Result == EOS_EResult::EOS_Success)
        {
            // Update the lobby with the new attribute
            EOS_Lobby_UpdateLobbyOptions UpdateLobbyOptions;
            UpdateLobbyOptions.ApiVersion = EOS_LOBBY_UPDATELOBBY_API_LATEST;
            UpdateLobbyOptions.LobbyModificationHandle = LobbyModificationHandle;

            EOS_Lobby_UpdateLobby(LobbyHandle, &UpdateLobbyOptions, nullptr, &ULobbySubsystem::OnLobbyUpdated);
        }
        else
            UE_LOG(LogLobbySubsystem, Warning, TEXT("Failed to add SteamInviterID attribute. Code: [%s]"), *FString(EOS_EResult_ToString(Result)));
    }
    else
        UE_LOG(LogLobbySubsystem, Warning, TEXT("Failed to get lobby modification handle. Code: [%s]"), *FString(EOS_EResult_ToString(Result)));
}
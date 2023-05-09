// Copyright © 2023 Melvin Brink

#include "EOS/Subsystems/LobbySubsystem.h"

#include "EOS/EOSManager.h"
#include "eos_lobby.h"



void ULobbySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	EosManager = &FEosManager::Get();
	const EOS_HPlatform PlatformHandle = EosManager->GetPlatformHandle();
	if(!PlatformHandle)
	{
		UE_LOG(LogLobbySubsystem, Error, TEXT("Platform-Handle is null"));
		return;
	}
	
	LobbyHandle = EOS_Platform_GetLobbyInterface(PlatformHandle);
}


// --------------------------------------------


void ULobbySubsystem::CreateLobby()
{
	EOS_Lobby_CreateLobbyOptions CreateLobbyOptions;
	CreateLobbyOptions.ApiVersion = EOS_LOBBY_CREATELOBBY_API_LATEST;
	CreateLobbyOptions.LocalUserId = EosManager->GetLocalUser().ProductUserId;
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
	if(Data->ResultCode == EOS_EResult::EOS_Success)
	{
		UE_LOG(LogLobbySubsystem, Warning, TEXT("Lobby created successfully"));

		// TODO: Check if a steam user created this lobby. If so, add the SteamInviterId attribute to the lobby.
		ULobbySubsystem* LobbySubsystem = static_cast<ULobbySubsystem*>(Data->ClientData);
		if(LobbySubsystem) LobbySubsystem->ApplySteamInviterAttributeModification(Data->LobbyId);
	}
	else
	{
		UE_LOG(LogLobbySubsystem, Warning, TEXT("Failed to create Lobby. Code: [%s]"), *FString(EOS_EResult_ToString(Data->ResultCode)));
	}
}

void ULobbySubsystem::OnLobbyUpdated(const EOS_Lobby_UpdateLobbyCallbackInfo* Data)
{
	if(Data->ResultCode == EOS_EResult::EOS_Success)
	{
		UE_LOG(LogLobbySubsystem, Warning, TEXT("Lobby updated successfully"));
	}
	else
	{
		UE_LOG(LogLobbySubsystem, Warning, TEXT("Failed to update Lobby. Code: [%s]"), *FString(EOS_EResult_ToString(Data->ResultCode)));
	}
}

/**
 * Creates a lobby modification where the SteamInviterId attribute is added.
 *
 * This attribute is needed for other steam users, invited through the Steam overlay, to join this EOS lobby.
 */
void ULobbySubsystem::ApplySteamInviterAttributeModification(const EOS_LobbyId LobbyId)
{
	// Get the lobby modification handle
    EOS_HLobbyModification LobbyModificationHandle = nullptr;
    EOS_Lobby_UpdateLobbyModificationOptions UpdateLobbyModificationOptions;
    UpdateLobbyModificationOptions.ApiVersion = EOS_LOBBY_UPDATELOBBYMODIFICATION_API_LATEST;
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
        {
            UE_LOG(LogLobbySubsystem, Warning, TEXT("Failed to add SteamInviterID attribute. Code: [%s]"), *FString(EOS_EResult_ToString(Result)));
        }
    }
    else
    {
        UE_LOG(LogLobbySubsystem, Warning, TEXT("Failed to get lobby modification handle. Code: [%s]"), *FString(EOS_EResult_ToString(Result)));
    }
}

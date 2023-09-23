// Copyright © 2023 Melvin Brink

#include "Subsystems/Lobby/LobbySubsystem.h"
#include "Subsystems/Lobby/SteamLobbySubsystem.h"
#include "Subsystems/Connect/ConnectSubsystem.h"
#include "Subsystems/User/Local/LocalUserSubsystem.h"
#include "Subsystems/User/Online/OnlineUserSubsystem.h"

#include "Types/UserTypes.h"
#include "Types/LobbyTypes.h"
#include "Helpers.h"
#include "EOSManager.h"
#include "eos_lobby.h"
#include "Subsystems/Session/SessionSubsystem.h"


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
	LobbyHandle = EOS_Platform_GetLobbyInterface(PlatformHandle);

	EOS_Lobby_AddNotifyLobbyUpdateReceivedOptions LobbyUpdateReceivedOptions;
	LobbyUpdateReceivedOptions.ApiVersion = EOS_LOBBY_ADDNOTIFYLOBBYUPDATERECEIVED_API_LATEST;
	OnLobbyUpdateNotification = EOS_Lobby_AddNotifyLobbyUpdateReceived(LobbyHandle, &LobbyUpdateReceivedOptions, this, &ThisClass::OnLobbyUpdate);
	
	// Register lobby member status update callback.
	EOS_Lobby_AddNotifyLobbyMemberStatusReceivedOptions LobbyMemberStatusReceivedOptions;
	LobbyMemberStatusReceivedOptions.ApiVersion = EOS_LOBBY_ADDNOTIFYLOBBYMEMBERSTATUSRECEIVED_API_LATEST;
	OnLobbyMemberStatusNotification = EOS_Lobby_AddNotifyLobbyMemberStatusReceived(LobbyHandle, &LobbyMemberStatusReceivedOptions, this, &ThisClass::OnLobbyMemberStatusUpdate);
}

void ULobbySubsystem::Deinitialize()
{
	EOS_Lobby_RemoveNotifyLobbyUpdateReceived(LobbyHandle, OnLobbyUpdateNotification);
	EOS_Lobby_RemoveNotifyLobbyMemberStatusReceived(LobbyHandle, OnLobbyMemberStatusNotification);

	Super::Deinitialize();
}


// --------------------------------------------


struct FCreateLobbyClientData
{
	ULobbySubsystem* Self;
	ULocalUserSubsystem* LocalUserSubsystem;
	int32 MaxMembers;
};

void ULobbySubsystem::CreateLobby(const int32 MaxMembers)
{
	if(ActiveLobby())
	{
		OnCreateLobbyCompleteDelegate.Broadcast(ECreateLobbyResultCode::InLobby, Lobby);
		return;
	}

	// Lobby Settings
	EOS_Lobby_CreateLobbyOptions CreateLobbyOptions;
	CreateLobbyOptions.ApiVersion = EOS_LOBBY_CREATELOBBY_API_LATEST;
	CreateLobbyOptions.LocalUserId = EosProductIDFromString(LocalUserSubsystem->GetLocalUser()->GetProductUserID());
	CreateLobbyOptions.MaxLobbyMembers = MaxMembers;
	CreateLobbyOptions.PermissionLevel = EOS_ELobbyPermissionLevel::EOS_LPL_INVITEONLY; // Todo option setting
	CreateLobbyOptions.bPresenceEnabled = true;
	CreateLobbyOptions.bAllowInvites = true;
	CreateLobbyOptions.BucketId = "PresenceLobby";
	CreateLobbyOptions.bDisableHostMigration = false;
	CreateLobbyOptions.bEnableRTCRoom = false;
	CreateLobbyOptions.LocalRTCOptions = nullptr;
	CreateLobbyOptions.LobbyId = nullptr;
	CreateLobbyOptions.bEnableJoinById = true;
	CreateLobbyOptions.bRejoinAfterKickRequiresInvite = true;
	CreateLobbyOptions.AllowedPlatformIds = nullptr;
	CreateLobbyOptions.AllowedPlatformIdsCount = 0;
	CreateLobbyOptions.bCrossplayOptOut = false;
	
	// ClientData to pass to the callback
	FCreateLobbyClientData* CreateLobbyClientData = new FCreateLobbyClientData();
	CreateLobbyClientData->Self = this;
	CreateLobbyClientData->LocalUserSubsystem = GetGameInstance()->GetSubsystem<ULocalUserSubsystem>();
	CreateLobbyClientData->MaxMembers = MaxMembers;

	// Create the EOS lobby and handle the result
	EOS_Lobby_CreateLobby(LobbyHandle, &CreateLobbyOptions, CreateLobbyClientData, [](const EOS_Lobby_CreateLobbyCallbackInfo* Data)
    {
		const FCreateLobbyClientData* ClientData = static_cast<FCreateLobbyClientData*>(Data->ClientData);
        ULobbySubsystem* LobbySubsystem = ClientData->Self;
		const ULocalUserSubsystem* LocalUserSubsystem = ClientData->LocalUserSubsystem;
		ULocalUser* LocalUser = LocalUserSubsystem->GetLocalUser();

		// EOS_LobbyId to FString
		FString LobbyID;
		if(Data->LobbyId) LobbyID = FString(Data->LobbyId);
		
		if(Data->ResultCode == EOS_EResult::EOS_Success)
		{
			LobbySubsystem->Lobby.Reset(); // Set everything to default to be sure.
			
			// Set the lobby data.
			LobbySubsystem->Lobby.ID = LobbyID;
			LobbySubsystem->Lobby.OwnerID = LocalUser->GetProductUserID();
			LobbySubsystem->Lobby.MemberList = TMap<FString, UOnlineUser*>();
			LobbySubsystem->Lobby.Settings.MaxMembers = ClientData->MaxMembers;

			// Broadcast success.
			LobbySubsystem->OnCreateLobbyCompleteDelegate.Broadcast(ECreateLobbyResultCode::Success, LobbySubsystem->Lobby);
			
			// Create a shadow-lobby on the user's local-platform.
			if(LocalUser->GetPlatform() == EPlatform::Epic) return;
			LobbySubsystem->LocalPlatformLobbySubsystem->OnCreateShadowLobbyCompleteDelegate.BindUObject(LobbySubsystem, &ULobbySubsystem::OnCreateShadowLobbyComplete);
			LobbySubsystem->LocalPlatformLobbySubsystem->CreateLobby();
		}
		else if(Data->ResultCode == EOS_EResult::EOS_Lobby_PresenceLobbyExists)
		{
			// If a 'Presence Lobby' exists, then join this lobby using your User-ID.
			LobbySubsystem->OnCreateLobbyCompleteDelegate.Broadcast(ECreateLobbyResultCode::PresenceLobbyExists, LobbySubsystem->Lobby);
		}
		else
		{
			UE_LOG(LogLobbySubsystem, Warning, TEXT("Failed to create Lobby. Code: [%s]"), *FString(EOS_EResult_ToString(Data->ResultCode)));
			LobbySubsystem->OnCreateLobbyCompleteDelegate.Broadcast(ECreateLobbyResultCode::Unknown, LobbySubsystem->Lobby);
		}
		
		delete ClientData;
    });
}

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
		OnJoinLobbyCompleteDelegate.Broadcast(EJoinLobbyResultCode::EosFailure, Lobby);
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
				LobbySubsystem->OnJoinLobbyCompleteDelegate.Broadcast(EJoinLobbyResultCode::Unknown, LobbySubsystem->Lobby);
			}
		}
		else if(Data->ResultCode == EOS_EResult::EOS_NotFound)
		{
			UE_LOG(LogLobbySubsystem, Log, TEXT("No lobbies found."));
			LobbySubsystem->OnJoinLobbyCompleteDelegate.Broadcast(EJoinLobbyResultCode::NotFound, LobbySubsystem->Lobby);
		}
		else
		{
			UE_LOG(LogLobbySubsystem, Warning, TEXT("Failed to find a lobby. Result-Code: [%s]"), *FString(EOS_EResult_ToString(Data->ResultCode)));
			LobbySubsystem->OnJoinLobbyCompleteDelegate.Broadcast(EJoinLobbyResultCode::Unknown, LobbySubsystem->Lobby);
		}
	});
}

void ULobbySubsystem::LeaveLobby()
{
	if(!ActiveLobby() || !LobbyHandle)
	{
		UE_LOG(LogLobbySubsystem, Error, TEXT("Cannot leave a lobby when not in one."));
		OnLeaveLobbyCompleteDelegate.Broadcast(ELeaveLobbyResultCode::NotInLobby);
		return;
	}
	
	EOS_Lobby_LeaveLobbyOptions LeaveLobbyOptions;
	LeaveLobbyOptions.ApiVersion = EOS_LOBBY_LEAVELOBBY_API_LATEST;
	LeaveLobbyOptions.LocalUserId = EosProductIDFromString(LocalUserSubsystem->GetLocalUser()->GetProductUserID());
	LeaveLobbyOptions.LobbyId = TCHAR_TO_UTF8(*Lobby.ID);
	
	EOS_Lobby_LeaveLobby(LobbyHandle, &LeaveLobbyOptions, this, [](const EOS_Lobby_LeaveLobbyCallbackInfo* Data)
	{
		ULobbySubsystem* LobbySubsystem = static_cast<ULobbySubsystem*>(Data->ClientData);

		// Clear lobby data
		LobbySubsystem->Lobby.Reset();
		
		if(Data->ResultCode == EOS_EResult::EOS_Success || Data->ResultCode == EOS_EResult::EOS_NotFound)
		{
			// Leave shadow lobby as well.
			LobbySubsystem->LocalPlatformLobbySubsystem->LeaveLobby();
			LobbySubsystem->OnLeaveLobbyCompleteDelegate.Broadcast(ELeaveLobbyResultCode::Success);
		}
		else
		{
			UE_LOG(LogLobbySubsystem, Error, TEXT("Failed to leave the lobby. Result-Code: [%s]"), *FString(EOS_EResult_ToString(Data->ResultCode)))
			LobbySubsystem->OnLeaveLobbyCompleteDelegate.Broadcast(ELeaveLobbyResultCode::Failure);
		}
	});
}

void ULobbySubsystem::StartListenServer(UObject* WorldContextObject, FLatentActionInfos LatentInfos)
{
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

	if(Data->ResultCode == EOS_EResult::EOS_Success || Data->ResultCode == EOS_EResult::EOS_Lobby_PresenceLobbyExists)
	{
		// Successfully joined the lobby, or we were already part of the lobby.
		LobbySubsystem->Lobby.ID = FString(Data->LobbyId);

		// Load the lobby.
		// TODO: Change name, to LoadLobby? GetLobbyInfo? And also get the attributes in this function.
		LobbySubsystem->LoadLobby([LobbySubsystem](const bool bSuccess)
		{
			if(bSuccess)
			{
				LobbySubsystem->OnJoinLobbyCompleteDelegate.Broadcast(EJoinLobbyResultCode::Success, LobbySubsystem->Lobby);
				// TODO: Check if shadow lobby exist, if not create one.
			}
			else
			{
				// TODO: Why did this fail?
				UE_LOG(LogLobbySubsystem, Error, TEXT("Failed to load the details about this lobby."));
				LobbySubsystem->OnJoinLobbyCompleteDelegate.Broadcast(EJoinLobbyResultCode::Failure, LobbySubsystem->Lobby); // Change this ELobbyResultCode::JoinFailure to false if there is a case where this may fail, then also leave the lobby.
				LobbySubsystem->LeaveLobby();
			}
		});
	}
	else
	{
		// TODO: EOS_NotFound
		UE_LOG(LogLobbySubsystem, Warning, TEXT("Failed to join lobby. ResultCode: [%s]"), *FString(EOS_EResult_ToString(Data->ResultCode)));
		LobbySubsystem->OnJoinLobbyCompleteDelegate.Broadcast(EJoinLobbyResultCode::Failure, LobbySubsystem->Lobby);
	}
}


// --------------------------------------------

/**
 * Data needed in EOS_Lobby_UpdateLobby after updating an attribute.
 */
struct FSetAttributeCompleteClientData
{
	ULobbySubsystem* Self;
	TArray<FLobbyAttribute> ChangedAttributes;
	TFunction<void(const bool bWasSuccessful)> OnCompleteCallback;
};

/**
 * Set/update multiple attributes on the lobby.
 */
void ULobbySubsystem::SetAttributes(TArray<FLobbyAttribute> Attributes, TFunction<void(const bool bWasSuccessful)> OnCompleteCallback)
{
	// Can only update the lobby-attributes if owner.
	if(Lobby.OwnerID != LocalUserSubsystem->GetLocalUser()->GetProductUserID())
	{
		UE_LOG(LogLobbySubsystem, Error, TEXT("Only the lobby owner can set its attributes."));
		return;
	}

	// Return if given attributes contain a special attribute, which is not allowed.
	// for (const auto& Attribute : Attributes)
	// {
	// 	if (SpecialAttributes.Contains(Attribute.Key))
	// 	{
	// 		UE_LOG(LogLobbySubsystem, Warning, TEXT("'%s' is a special attribute that should not be set using ::SetAttributes, use the corresponding method for it instead."), *Attribute.Key);
	// 		if(OnCompleteCallback) OnCompleteCallback(false);
	// 		return;
	// 	}
	// }

	// Only set/update attributes that unchanged, and are non-special attributes (reserved attributes for specific functionality).
	const TArray<FLobbyAttribute> ChangedAttributes = FilterAttributes(Attributes);
	if (!ChangedAttributes.Num())
	{
		if(OnCompleteCallback) OnCompleteCallback(true);
		return;
	}
	
	// Options for creating the Modification-Handle
	EOS_Lobby_UpdateLobbyModificationOptions UpdateLobbyModificationOptions;
	UpdateLobbyModificationOptions.ApiVersion = EOS_LOBBY_UPDATELOBBYMODIFICATION_API_LATEST;
	UpdateLobbyModificationOptions.LocalUserId = EosProductIDFromString(LocalUserSubsystem->GetLocalUser()->GetProductUserID());
	UpdateLobbyModificationOptions.LobbyId = TCHAR_TO_ANSI(*Lobby.ID);

	EOS_HLobbyModification LobbyModificationHandle;
	if (EOS_EResult Result = EOS_Lobby_UpdateLobbyModification(LobbyHandle, &UpdateLobbyModificationOptions, &LobbyModificationHandle); Result == EOS_EResult::EOS_Success)
	{
		const TArray<FLobbyAttribute> SuccessfulAttributes = AddAttributeOnModificationHandle(LobbyModificationHandle, ChangedAttributes);
		
		// Update the lobby with the Handle.
		EOS_Lobby_UpdateLobbyOptions UpdateLobbyOptions;
		UpdateLobbyOptions.ApiVersion = EOS_LOBBY_UPDATELOBBY_API_LATEST;
		UpdateLobbyOptions.LobbyModificationHandle = LobbyModificationHandle;
		
		FSetAttributeCompleteClientData* SetAttributeCompleteClientData = new FSetAttributeCompleteClientData{this, SuccessfulAttributes, OnCompleteCallback};
		EOS_Lobby_UpdateLobby(LobbyHandle, &UpdateLobbyOptions, SetAttributeCompleteClientData, [](const EOS_Lobby_UpdateLobbyCallbackInfo* Data)
		{
			const FSetAttributeCompleteClientData* ClientData = static_cast<FSetAttributeCompleteClientData*>(Data->ClientData);
			
			if(Data->ResultCode == EOS_EResult::EOS_Success)
			{
				// Cache the updated attribute on the lobby.
				TArray<FLobbyAttribute> ChangedAttributes = ClientData->ChangedAttributes;
				for (FLobbyAttribute Attribute : ChangedAttributes) ClientData->Self->Lobby.Attributes.Add(Attribute.Key, Attribute);
				
				UE_LOG(LogLobbySubsystem, Log, TEXT("Lobby Attribute(s) successfully added."))
				if(ClientData->OnCompleteCallback)  ClientData->OnCompleteCallback(true);
			}
			else
			{
				UE_LOG(LogLobbySubsystem, Error, TEXT("Failed to update the lobby with the new attribute(s). Result-Code: [%s]"), *FString(EOS_EResult_ToString(Data->ResultCode)));
				if(ClientData->OnCompleteCallback) ClientData->OnCompleteCallback(false);
			}

			delete ClientData;
		});

		// Release the memory of the Handle.
		EOS_LobbyModification_Release(LobbyModificationHandle);
	}
	else
	{
		UE_LOG(LogLobbySubsystem, Error, TEXT("Failed to create the lobby-modification-handle for setting the attribute(s). Result-Code: [%s]"), *FString(EOS_EResult_ToString(Result)));
		if(OnCompleteCallback) OnCompleteCallback(false);
	}
}

TArray<FLobbyAttribute> ULobbySubsystem::FilterAttributes(TArray<FLobbyAttribute>& Attributes)
{
	TArray<FLobbyAttribute> ChangedAttributes;
	for (const auto& Attribute : Attributes)
	{
		const FLobbyAttribute* ExistingAttribute = Lobby.Attributes.Find(Attribute.Key);
		if (!ExistingAttribute)
		{
			ChangedAttributes.Add(Attribute);
			continue;
		}

		bool bShouldAdd = false;
		switch (ExistingAttribute->Type)
		{
		case ELobbyAttributeType::Bool:
			bShouldAdd = ExistingAttribute->BoolValue != Attribute.BoolValue;
			break;
		case ELobbyAttributeType::String:
			bShouldAdd = ExistingAttribute->StringValue != Attribute.StringValue;
			break;
		case ELobbyAttributeType::Int64:
			bShouldAdd = ExistingAttribute->IntValue != Attribute.IntValue;
			break;
		case ELobbyAttributeType::Double:
			bShouldAdd = ExistingAttribute->DoubleValue != Attribute.DoubleValue;
			break;
		}
		if (bShouldAdd) ChangedAttributes.Add(Attribute);
	}
	return ChangedAttributes;
}

TArray<FLobbyAttribute> ULobbySubsystem::AddAttributeOnModificationHandle(EOS_HLobbyModification& LobbyModificationHandle, const TArray<FLobbyAttribute>& Attributes)
{
	// Loop through all attributes and add them to the Handle
	TArray<FLobbyAttribute> SuccessfulAttributes;
	for (const FLobbyAttribute Attribute : Attributes)
	{
		EOS_Lobby_AttributeData EosAttributeData;
		EosAttributeData.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
		EosAttributeData.Key = TCHAR_TO_ANSI(*Attribute.Key);
			
		switch (Attribute.Type)
		{
		case ELobbyAttributeType::Bool:
			EosAttributeData.ValueType = EOS_ELobbyAttributeType::EOS_AT_BOOLEAN;
			EosAttributeData.Value.AsBool = Attribute.BoolValue ? EOS_TRUE : EOS_FALSE;
			break;
		case ELobbyAttributeType::String:
			EosAttributeData.ValueType = EOS_ELobbyAttributeType::EOS_AT_STRING;
			EosAttributeData.Value.AsUtf8 = TCHAR_TO_UTF8(*Attribute.StringValue);
			break;
		case ELobbyAttributeType::Int64:
			EosAttributeData.ValueType = EOS_ELobbyAttributeType::EOS_AT_INT64;
			EosAttributeData.Value.AsInt64 = Attribute.IntValue;
			break;
		case ELobbyAttributeType::Double:
			EosAttributeData.ValueType = EOS_ELobbyAttributeType::EOS_AT_DOUBLE;
			EosAttributeData.Value.AsDouble = Attribute.DoubleValue;
			break;
		}

		EOS_LobbyModification_AddAttributeOptions AttributeOptions;
		AttributeOptions.ApiVersion = EOS_LOBBYMODIFICATION_ADDATTRIBUTE_API_LATEST;
		AttributeOptions.Attribute = &EosAttributeData;
		AttributeOptions.Visibility = EOS_ELobbyAttributeVisibility::EOS_LAT_PUBLIC;

		// Add the change to the Handle.
		if (const EOS_EResult Result = EOS_LobbyModification_AddAttribute(LobbyModificationHandle, &AttributeOptions); Result != EOS_EResult::EOS_Success)
		{
			UE_LOG(LogLobbySubsystem, Log, TEXT("Failed to add an attribute to the LobbyModification. Result-Code: [%s]"), *FString(EOS_EResult_ToString(Result)));
			continue;
		}

		SuccessfulAttributes.Add(Attribute);
	}
	return SuccessfulAttributes;
}

/**
 * Get the latest lobby attributes from the EOS Lobby.
 *
 * Should only be used to get new attributes after a lobby-update has occured, otherwise use the cached attributes on FLobby.
 */
TArray<FLobbyAttribute> ULobbySubsystem::GetAttributesFromDetailsHandle()
{
	const EOS_HLobbyDetails LobbyDetailsHandle = GetLobbyDetailsHandle();
	if (!LobbyDetailsHandle)
	{
		UE_LOG(LogLobbySubsystem, Error, TEXT("Invalid LobbyDetailsHandle received from ::GetLobbyDetailsHandle"));
		return TArray<FLobbyAttribute>();
	}
	
	// Get the attribute count.
	EOS_LobbyDetails_GetAttributeCountOptions AttributeCountOptions;
	AttributeCountOptions.ApiVersion = EOS_LOBBYDETAILS_GETATTRIBUTECOUNT_API_LATEST;
	const uint32_t AttributeCount = EOS_LobbyDetails_GetAttributeCount(LobbyDetailsHandle, &AttributeCountOptions);

	// Get all attributes using the count.
	TArray<FLobbyAttribute> LobbyAttributes;
	for (uint32_t AttributeIndex = 0; AttributeIndex < AttributeCount; ++AttributeIndex)
	{
		EOS_LobbyDetails_CopyAttributeByIndexOptions AttributeOptions;
		AttributeOptions.ApiVersion = EOS_LOBBYDETAILS_COPYATTRIBUTEBYINDEX_API_LATEST;
		AttributeOptions.AttrIndex = AttributeIndex;

		EOS_Lobby_Attribute* EosAttribute;
		const EOS_EResult CopiedAttributeResult = EOS_LobbyDetails_CopyAttributeByIndex(LobbyDetailsHandle, &AttributeOptions, &EosAttribute);
		if (CopiedAttributeResult != EOS_EResult::EOS_Success || !EosAttribute || !EosAttribute->Data)
		{
			UE_LOG(LogLobbySubsystem, Error, TEXT("Failed to copy an Attribute by Index in ::GetAttributesFromDetailsHandle."));
			continue;
		}
		
		// Create the attribute and set it to the correct type.
		FLobbyAttribute LobbyAttribute;
		LobbyAttribute.Key = EosAttribute->Data->Key;
		
		switch (EosAttribute->Data->ValueType)
		{
		case EOS_ELobbyAttributeType::EOS_AT_BOOLEAN:
			LobbyAttribute.Type = ELobbyAttributeType::Bool;
			LobbyAttribute.BoolValue = EosAttribute->Data->Value.AsBool == EOS_TRUE;
			break;
		case EOS_ELobbyAttributeType::EOS_AT_STRING:
			LobbyAttribute.Type = ELobbyAttributeType::String;
			LobbyAttribute.StringValue = ANSI_TO_TCHAR(EosAttribute->Data->Value.AsUtf8);
			break;
		case EOS_ELobbyAttributeType::EOS_AT_INT64:
			LobbyAttribute.Type = ELobbyAttributeType::Int64;
			LobbyAttribute.IntValue = EosAttribute->Data->Value.AsInt64;
			break;
		case EOS_ELobbyAttributeType::EOS_AT_DOUBLE:
			LobbyAttribute.Type = ELobbyAttributeType::Double;
			LobbyAttribute.DoubleValue = EosAttribute->Data->Value.AsDouble;
			break;
		}
		
		LobbyAttributes.Add(LobbyAttribute);
		EOS_Lobby_Attribute_Release(EosAttribute);
	}

	EOS_LobbyDetails_Release(LobbyDetailsHandle);
	return LobbyAttributes;
}


// --------------------------------------------


void ULobbySubsystem::OnLobbyUpdate(const EOS_Lobby_LobbyUpdateReceivedCallbackInfo* Data)
{
    ULobbySubsystem* LobbySubsystem = static_cast<ULobbySubsystem*>(Data->ClientData);
    UE_LOG(LogLobbySubsystem, Log, TEXT("Lobby update received.")) // todo: callback gets called multiple times, try to solve.

	TMap<FString, FLobbyAttribute> CachedAttributes = LobbySubsystem->Lobby.Attributes;
    TArray<FLobbyAttribute> LatestAttributes = LobbySubsystem->GetAttributesFromDetailsHandle();

	// Check each attribute if it has changed.
    for (const FLobbyAttribute LatestAttribute : LatestAttributes)
    {
    	// Check for any special attribute changes, otherwise update the custom attribute.
    	const FLobbyAttribute* CachedAttribute = CachedAttributes.Find(LatestAttribute.Key);
    	bool bHasChanged = false;

    	if (CachedAttribute)
    	{
    		// Check if attributes differ.
    		switch (LatestAttribute.Type)
    		{
    		case ELobbyAttributeType::Bool:
    			bHasChanged = !(CachedAttribute->Type == ELobbyAttributeType::Bool && CachedAttribute->BoolValue == LatestAttribute.BoolValue);
    			break;
    		case ELobbyAttributeType::String:
    			bHasChanged = !(CachedAttribute->Type == ELobbyAttributeType::String && CachedAttribute->StringValue == LatestAttribute.StringValue);
    			break;
    		case ELobbyAttributeType::Int64:
    			bHasChanged = !(CachedAttribute->Type == ELobbyAttributeType::Int64 && CachedAttribute->IntValue == LatestAttribute.IntValue);
    			break;
    		case ELobbyAttributeType::Double:
    			bHasChanged = !(CachedAttribute->Type == ELobbyAttributeType::Double && CachedAttribute->DoubleValue == LatestAttribute.DoubleValue);
    			break;
    		}
    	}
    	else bHasChanged = true; // The attribute doesn't exist in the cache.

    	// Set/update the attribute on the lobby if it has changed.
    	if (bHasChanged) LobbySubsystem->Lobby.Attributes.Emplace(LatestAttribute.Key, LatestAttribute);
    	else continue;
    	
    	// Broadcast corresponding delegate.
    	if(const FString Key = LatestAttribute.Key; Key == "ServerAddress")
    	{
    		ULocalUserSubsystem* LocalUserSubsystem = LobbySubsystem->GetGameInstance()->GetSubsystem<ULocalUserSubsystem>();
    		
    		// If the Server-Address is set, it means that the host wants to start the server and is waiting for members to join.
    		if(!LatestAttribute.StringValue.IsEmpty())
    		{
    			// Don't broadcast to the owner of the lobby.
    			if(LobbySubsystem->Lobby.OwnerID != LocalUserSubsystem->GetLocalUser()->GetProductUserID()) LobbySubsystem->OnLobbyStartedDelegate.Broadcast(LatestAttribute.StringValue);
    		}
    		else LobbySubsystem->OnLobbyStoppedDelegate.Broadcast();
    	}else
    	{
    		LobbySubsystem->OnLobbyAttributeChanged.Broadcast(LatestAttribute);
    	}
    }
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
	
	// Add the user to the list of users to load.
	UsersToLoad.Add(TargetUserID);

	// Fetch the info of this user.
	OnlineUserSubsystem->GetOnlineUser(TargetUserID, [this, TargetUserID](const FGetOnlineUserResult Result)
	{
		// Check if user is still in lobby after this fetch
		if(UsersToLoad.Contains(TargetUserID))
		{
			Lobby.AddMember(Result.OnlineUser);
			OnLobbyUserJoinedDelegate.Broadcast(Result.OnlineUser);
		}
		else UE_LOG(LogLobbySubsystem, Log, TEXT("User left before we could load their data. OnLobbyUserJoinedDelegate will not be broadcasted."));
	});
}

void ULobbySubsystem::OnLobbyUserLeft(const FString& TargetUserID)
{
	UE_LOG(LogLobbySubsystem, Log, TEXT("A user has left the lobby"));

	Lobby.RemoveMember(TargetUserID);
	OnLobbyUserLeftDelegate.Broadcast(TargetUserID); 
}

void ULobbySubsystem::OnLobbyUserDisconnected(const FString& TargetUserID)
{
	UE_LOG(LogLobbySubsystem, Log, TEXT("A user has unexpectedly left the lobby"));

	Lobby.RemoveMember(TargetUserID);
	OnLobbyUserDisconnectedDelegate.Broadcast(TargetUserID);
}

void ULobbySubsystem::OnLobbyUserKicked(const FString& TargetUserID)
{
	UE_LOG(LogLobbySubsystem, Log, TEXT("A user has been kicked from the lobby"));

	Lobby.RemoveMember(TargetUserID);
	OnLobbyUserKickedDelegate.Broadcast(TargetUserID);
}

void ULobbySubsystem::OnLobbyUserPromoted(const FString& TargetUserID)
{
	UE_LOG(LogLobbySubsystem, Log, TEXT("A user has been promoted to Lobby-Owner"));

	Lobby.OwnerID = TargetUserID;
	OnLobbyUserPromotedDelegate.Broadcast(TargetUserID);
}


// --------------------------------------------


/*
 * Remember to release the handle after using it with EOS_LobbyDetails_Release().
 */
EOS_HLobbyDetails ULobbySubsystem::GetLobbyDetailsHandle() const
{
	EOS_Lobby_CopyLobbyDetailsHandleOptions LobbyDetailsOptions;
	LobbyDetailsOptions.ApiVersion = EOS_LOBBY_COPYLOBBYDETAILSHANDLE_API_LATEST;
	const std::string ConvertedLobbyID = TCHAR_TO_UTF8(*Lobby.ID);
	LobbyDetailsOptions.LobbyId = ConvertedLobbyID.c_str();
	LobbyDetailsOptions.LocalUserId = EosProductIDFromString(LocalUserSubsystem->GetLocalUser()->GetProductUserID());
	
	EOS_HLobbyDetails LobbyDetailsHandle;
	const EOS_EResult Result = EOS_Lobby_CopyLobbyDetailsHandle(LobbyHandle, &LobbyDetailsOptions, &LobbyDetailsHandle);
	if(Result == EOS_EResult::EOS_Success) return LobbyDetailsHandle;
	
	UE_LOG(LogLobbySubsystem, Warning, TEXT("Failed to get the Lobby-Details-Handle."));
	return nullptr;
}

/**
 * Loads all the necessary information from the lobby and stores it on the lobby-subsystems.
 *
 * @param OnCompleteCallback Called when the function completes.
 */
void ULobbySubsystem::LoadLobby(TFunction<void(bool bSuccess)> OnCompleteCallback)
{
	if(!ActiveLobby())
	{
		UE_LOG(LogLobbySubsystem, Log, TEXT("Must be in a lobby first before calling ::LoadLobby"));
		return;
	}

	const EOS_HLobbyDetails LobbyDetailsHandle = GetLobbyDetailsHandle();
	if(!LobbyDetailsHandle) OnCompleteCallback(false);
	
	// Get Lobby Info
	constexpr EOS_LobbyDetails_CopyInfoOptions CopyInfoOptions{ EOS_LOBBYDETAILS_COPYINFO_API_LATEST };
	EOS_LobbyDetails_Info* LobbyInfo;
	if(const EOS_EResult Result = EOS_LobbyDetails_CopyInfo(LobbyDetailsHandle, &CopyInfoOptions, &LobbyInfo); Result == EOS_EResult::EOS_Success)
	{
		// Store the details we need
		Lobby.ID = FString(LobbyInfo->LobbyId);
		Lobby.OwnerID = EosProductIDToString(LobbyInfo->LobbyOwnerUserId);
		Lobby.Settings.MaxMembers = LobbyInfo->MaxMembers;

		TArray<FLobbyAttribute> AttributeList = GetAttributesFromDetailsHandle();
		TMap<FString, FLobbyAttribute> AttributeMap;
		for (FLobbyAttribute Attribute : AttributeList) AttributeMap.Add(Attribute.Key, Attribute);
		Lobby.Attributes = AttributeMap;
		
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
			Lobby.MemberList = OnlineUserMap;

			// Done loading data.
			OnCompleteCallback(true);
		});
	}
	else
	{
		EOS_LobbyDetails_Release(LobbyDetailsHandle);
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
    UpdateLobbyModificationOptions.LobbyId = TCHAR_TO_ANSI(*Lobby.ID);
 
	// Create the lobby modification handle.
	EOS_HLobbyModification LobbyModificationHandle;
    if (EOS_EResult Result = EOS_Lobby_UpdateLobbyModification(LobbyHandle, &UpdateLobbyModificationOptions, &LobbyModificationHandle); Result == EOS_EResult::EOS_Success)
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
        if (Result = EOS_LobbyModification_AddAttribute(LobbyModificationHandle, &ShadowLobbyIDAttributeOptions); Result == EOS_EResult::EOS_Success)
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
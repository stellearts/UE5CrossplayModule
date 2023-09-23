// Copyright © 2023 Melvin Brink

#include "Subsystems/Session/SessionSubsystem.h"
#include "Subsystems/User/Local/LocalUserSubsystem.h"
#include "Subsystems/Lobby/LobbySubsystem.h"
#include "EOSManager.h"
#include "eos_sessions.h"
#include "Helpers.h"
#include "GameModes/MultiplayerGameMode.h"



void USessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Make sure the local user state subsystem is initialized.
	LocalUserSubsystem = Collection.InitializeDependency<ULocalUserSubsystem>();
	LobbySubsystem = Collection.InitializeDependency<ULobbySubsystem>();

	EosManager = &FEosManager::Get();
	const EOS_HPlatform PlatformHandle = EosManager->GetPlatformHandle();
	if(!PlatformHandle) return;
	SessionHandle = EOS_Platform_GetSessionsInterface(PlatformHandle);

	// Invite received callback.
	EOS_Sessions_AddNotifySessionInviteReceivedOptions AddNotifySessionInviteReceivedOptions;
	AddNotifySessionInviteReceivedOptions.ApiVersion = EOS_SESSIONS_ADDNOTIFYSESSIONINVITERECEIVED_API_LATEST;
	OnSessionInviteNotification = EOS_Sessions_AddNotifySessionInviteReceived(SessionHandle, &AddNotifySessionInviteReceivedOptions, this, &ThisClass::OnInviteReceived);
}

void USessionSubsystem::Deinitialize()
{
	EOS_Sessions_RemoveNotifySessionInviteReceived(SessionHandle, OnSessionInviteNotification);
	
	Super::Deinitialize();
}


// --------------------------------------------

struct FCreateSessionClientData
{
	USessionSubsystem* Self;
};

void USessionSubsystem::CreateSession(const FSessionSettings& Settings)
{
	if(LobbySubsystem->ActiveLobby())
	{
		if(LobbySubsystem->GetLobby().OwnerID != LocalUserSubsystem->GetLocalUser()->GetProductUserID())
		{
			UE_LOG(LogSessionSubsystem, Log, TEXT("Cannot create a session when in a lobby and not being the owner."));
			OnCreateSessionCompleteDelegate.Broadcast(ECreateSessionResultCode::Failure, Session);
			return;
		}
		if(LobbySubsystem->GetLobby().GetMemberList().Num() > Settings.MaxMembers)
		{
			UE_LOG(LogSessionSubsystem, Log, TEXT("Cannot create a session with fewer slots than number of lobby members."));
			OnCreateSessionCompleteDelegate.Broadcast(ECreateSessionResultCode::Failure, Session);
			return;
		}
	}

	const AMultiplayerGameMode* MultiplayerGameMode = Cast<AMultiplayerGameMode>(GetWorld()->GetAuthGameMode());
	if(!MultiplayerGameMode)
	{
		UE_LOG(LogSessionSubsystem, Log, TEXT("Can only create a session when using a GameMode derived from 'AMultiplayerGameMode'."));
		return;
	}

	// UE_LOG(LogSessionSubsystem, Log, TEXT("Starting listen server..."))
	// GetWorld()->ServerTravel("?listen");

	
	// The following code will be added to a callback called after the server-travel completes.

	EOS_Sessions_CreateSessionModificationOptions CreateSessionOptions;
	CreateSessionOptions.ApiVersion = EOS_SESSIONS_CREATESESSIONMODIFICATION_API_LATEST;
	CreateSessionOptions.SessionName = TCHAR_TO_UTF8(*Settings.Name);
	CreateSessionOptions.BucketId = "Game:1.0.0";
	CreateSessionOptions.MaxPlayers = Settings.MaxMembers;
	CreateSessionOptions.LocalUserId = EosProductIDFromString(LocalUserSubsystem->GetLocalUser()->GetProductUserID());
	CreateSessionOptions.bPresenceEnabled = true;
	CreateSessionOptions.bSanctionsEnabled = false;
	
	EOS_HSessionModification SessionModification;
	const EOS_EResult Result = EOS_Sessions_CreateSessionModification(SessionHandle, &CreateSessionOptions, &SessionModification);
	if (Result == EOS_EResult::EOS_Success)
	{
		EOS_Sessions_UpdateSessionOptions UpdateSessionOptions;
		UpdateSessionOptions.ApiVersion = EOS_SESSIONS_UPDATESESSION_API_LATEST;
		UpdateSessionOptions.SessionModificationHandle = SessionModification;

		FCreateSessionClientData* CreateSessionClientData = new FCreateSessionClientData{this};
		EOS_Sessions_UpdateSession(SessionHandle, &UpdateSessionOptions, this, [](const EOS_Sessions_UpdateSessionCallbackInfo* Data)
		{
			const FCreateSessionClientData* ClientData = static_cast<FCreateSessionClientData*>(Data->ClientData);
			USessionSubsystem* SessionSubsystem = ClientData->Self;
			
			if(Data->ResultCode == EOS_EResult::EOS_Success)
			{
				SessionSubsystem->Session.Reset(); // Set everything to default to be sure.
				SessionSubsystem->Session.ID = Data->SessionId;
				SessionSubsystem->Session.Settings.Name = Data->SessionName;
				
				UE_LOG(LogSessionSubsystem, Warning, TEXT("Session created successfully."));
				SessionSubsystem->OnCreateSessionCompleteDelegate.Broadcast(ECreateSessionResultCode::Success, SessionSubsystem->Session);

				// If in a lobby, invite all its members to this session.
				if(ULobbySubsystem* LobbySubsystem = SessionSubsystem->LobbySubsystem; LobbySubsystem->ActiveLobby())
				{
					for (UOnlineUser* LobbyMember : LobbySubsystem->GetLobby().GetMemberList())
					{
						SessionSubsystem->InvitePlayer(LobbyMember->GetProductUserID());
					}
				}
			}
			else
			{
				UE_LOG(LogSessionSubsystem, Warning, TEXT("Failed to create session. Result-Code: [%s]"), *FString(EOS_EResult_ToString(Data->ResultCode)));
				SessionSubsystem->OnCreateSessionCompleteDelegate.Broadcast(ECreateSessionResultCode::Failure, SessionSubsystem->Session);
			}
		});

		// Release the session modification handle
		EOS_SessionModification_Release(SessionModification);
	}
	else
	{
		UE_LOG(LogSessionSubsystem, Warning, TEXT("Failed to create session modification. Code: [%s]"), *FString(EOS_EResult_ToString(Result)));
		OnCreateSessionCompleteDelegate.Broadcast(ECreateSessionResultCode::Failure, Session);
	}
}

struct FJoinSessionCompleteClientData
{
	USessionSubsystem* Self;
	EOS_HSessionDetails SessionDetailsHandle;
};

void USessionSubsystem::JoinSessionByHandle(const EOS_HSessionDetails& DetailsHandle)
{
	EOS_Sessions_JoinSessionOptions Options;
	Options.ApiVersion = EOS_SESSIONS_JOINSESSION_API_LATEST;
	Options.SessionName = "PresenceSession";
	Options.SessionHandle = DetailsHandle;
	Options.LocalUserId = EosProductIDFromString(LocalUserSubsystem->GetLocalUser()->GetProductUserID());
	Options.bPresenceEnabled = true;

	FJoinSessionCompleteClientData* JoinSessionCompleteClientData = new FJoinSessionCompleteClientData{this, DetailsHandle};
	EOS_Sessions_JoinSession(SessionHandle, &Options, JoinSessionCompleteClientData, &ThisClass::OnJoinSessionComplete);
}

void USessionSubsystem::OnJoinSessionComplete(const EOS_Sessions_JoinSessionCallbackInfo* Data)
{
	const FJoinSessionCompleteClientData* ClientData = static_cast<FJoinSessionCompleteClientData*>(Data->ClientData);
	USessionSubsystem* SessionSubsystem = ClientData->Self;
	
	if(Data->ResultCode == EOS_EResult::EOS_Success)
	{
		SessionSubsystem->SessionDetailsHandle = ClientData->SessionDetailsHandle;
		SessionSubsystem->LoadSession([SessionSubsystem](const bool bSuccess)
		{
			if(bSuccess)
			{
				// SessionSubsystem->OnJoinLobbyCompleteDelegate.Broadcast(EJoinLobbyResultCode::Success, SessionSubsystem->Session);
			}
			else
			{
				// TODO: Why did this fail?
				UE_LOG(LogSessionSubsystem, Error, TEXT("Failed to load the details about this session."));
				// SessionSubsystem->OnJoinLobbyCompleteDelegate.Broadcast(EJoinLobbyResultCode::Failure, SessionSubsystem->Session);
				// SessionSubsystem->LeaveLobby();
			}
		});
	}
	else UE_LOG(LogSessionSubsystem, Warning, TEXT("Failed to join the session. Result-Code: [%s]"), *FString(EOS_EResult_ToString(Data->ResultCode)));

	delete ClientData;
}

void USessionSubsystem::InvitePlayer(const FString& ProductUserID)
{
	EOS_Sessions_SendInviteOptions SendInviteOptions;
	SendInviteOptions.ApiVersion = EOS_SESSIONS_SENDINVITE_API_LATEST;
	SendInviteOptions.SessionName = TCHAR_TO_UTF8(*Session.Settings.Name);
	SendInviteOptions.LocalUserId = EosProductIDFromString(LocalUserSubsystem->GetLocalUser()->GetProductUserID());
	SendInviteOptions.TargetUserId = EosProductIDFromString(ProductUserID);
	
	EOS_Sessions_SendInvite(SessionHandle, &SendInviteOptions, nullptr, [](const EOS_Sessions_SendInviteCallbackInfo* Data)
	{
		if(Data->ResultCode != EOS_EResult::EOS_Success) UE_LOG(LogSessionSubsystem, Error, TEXT("Failed to send a session invite. Result-Code: [%s]"), *FString(EOS_EResult_ToString(Data->ResultCode)));
	});
}

void USessionSubsystem::OnInviteReceived(const EOS_Sessions_SessionInviteReceivedCallbackInfo* Data)
{
	USessionSubsystem* SessionSubsystem = static_cast<USessionSubsystem*>(Data->ClientData);
	const FString InviterProductUserID = EosProductIDToString(Data->TargetUserId);

	// If you are in a lobby and the owner of that lobby has sent you this invite, then join this session directly.
	if(SessionSubsystem->LobbySubsystem->ActiveLobby() && InviterProductUserID == SessionSubsystem->LobbySubsystem->GetLobby().OwnerID)
	{
		EOS_Sessions_CopySessionHandleByInviteIdOptions Options;
		Options.ApiVersion = EOS_SESSIONS_COPYSESSIONHANDLEBYINVITEID_API_LATEST;
		Options.InviteId = Data->InviteId;

		EOS_HSessionDetails SessionDetails;
		const EOS_EResult Result = EOS_Sessions_CopySessionHandleByInviteId(SessionSubsystem->SessionHandle, &Options, &SessionDetails);
		if(Result == EOS_EResult::EOS_Success)
		{
			SessionSubsystem->JoinSessionByHandle(SessionDetails);
			EOS_SessionDetails_Release(SessionDetails);
		}
		else UE_LOG(LogSessionSubsystem, Warning, TEXT("Failed to 'Copy Session Handle By Invite ID'. Result-Code: [%s]"), *FString(EOS_EResult_ToString(Result)));
	}

	// TODO: when being invited to a session by someone outside your lobby.
}

/**
 * Returns a new list of attributes which only includes the one's that have actually changed.
 * Will skip special attributes since they are reserved for specific functionality.
 */
TArray<FSessionAttribute> USessionSubsystem::FilterAttributes(TArray<FSessionAttribute> Attributes)
{
	TArray<FSessionAttribute> FilteredAttributes;
	for (const auto& Attribute : Attributes)
	{
		// Skip if it is a special attribute.
		if (SpecialAttributes.Contains(Attribute.Key))
		{
			UE_LOG(LogSessionSubsystem, Warning, TEXT("%s is a special attribute that should not be set using ::SetAttributes, use the corresponding method for it instead."), *Attribute.Key);
			continue;
		}
		
		const FSessionAttribute* ExistingAttribute = Session.Attributes.Find(Attribute.Key);

		// Add if it does not exist in the cache.
		if (!ExistingAttribute)
		{
			FilteredAttributes.Add(Attribute);
			continue;
		}

		// Add if value has differs from what is cached.
		bool bShouldAdd = false;
		switch (ExistingAttribute->Type)
		{
		case ESessionAttributeType::Bool:
			bShouldAdd = ExistingAttribute->BoolValue != Attribute.BoolValue;
			break;
		case ESessionAttributeType::String:
			bShouldAdd = ExistingAttribute->StringValue != Attribute.StringValue;
			break;
		case ESessionAttributeType::Int64:
			bShouldAdd = ExistingAttribute->IntValue != Attribute.IntValue;
			break;
		case ESessionAttributeType::Double:
			bShouldAdd = ExistingAttribute->DoubleValue != Attribute.DoubleValue;
			break;
		}
		if (bShouldAdd) FilteredAttributes.Add(Attribute);
	}
	return FilteredAttributes;
}

/**
 * Tries to add the given attribute on the given Handle, which is then used to update the lobby.
 * Returns true when successfully added the attribute on the Handle.
 */
bool USessionSubsystem::AddAttributeToHandle(EOS_HSessionModification& Handle, const FSessionAttribute& Attribute)
{
	EOS_Sessions_AttributeData EosAttributeData;
	EosAttributeData.ApiVersion = EOS_SESSIONS_ATTRIBUTEDATA_API_LATEST;
	EosAttributeData.Key = TCHAR_TO_ANSI(*Attribute.Key);
			
	switch (Attribute.Type)
	{
	case ESessionAttributeType::Bool:
		EosAttributeData.ValueType = EOS_ESessionAttributeType::EOS_AT_BOOLEAN;
		EosAttributeData.Value.AsBool = Attribute.BoolValue ? EOS_TRUE : EOS_FALSE;
		break;
	case ESessionAttributeType::String:
		EosAttributeData.ValueType = EOS_ESessionAttributeType::EOS_AT_STRING;
		EosAttributeData.Value.AsUtf8 = TCHAR_TO_UTF8(*Attribute.StringValue);
		break;
	case ESessionAttributeType::Int64:
		EosAttributeData.ValueType = EOS_ESessionAttributeType::EOS_AT_INT64;
		EosAttributeData.Value.AsInt64 = Attribute.IntValue;
		break;
	case ESessionAttributeType::Double:
		EosAttributeData.ValueType = EOS_ESessionAttributeType::EOS_AT_DOUBLE;
		EosAttributeData.Value.AsDouble = Attribute.DoubleValue;
		break;
	}

	EOS_SessionModification_AddAttributeOptions AttributeOptions;
	AttributeOptions.ApiVersion = EOS_SESSIONMODIFICATION_ADDATTRIBUTE_API_LATEST;
	AttributeOptions.SessionAttribute = &EosAttributeData;
	AttributeOptions.AdvertisementType = EOS_ESessionAttributeAdvertisementType::EOS_SAAT_Advertise;

	// Add the change to the Handle.
	if (const EOS_EResult Result = EOS_SessionModification_AddAttribute(Handle, &AttributeOptions); Result != EOS_EResult::EOS_Success)
	{
		UE_LOG(LogSessionSubsystem, Log, TEXT("Failed to add an attribute to the SessionModification-Handle. Result-Code: [%s]"), *FString(EOS_EResult_ToString(Result)));
		return false;
	}
	
	return true;
}

/**
 * Data needed in EOS_Sessions_UpdateSession after updating an attribute.
 */
struct FSetAttributesClientData
{
	USessionSubsystem* Self;
	TArray<FSessionAttribute> ChangedAttributes;
};

/**
 * Set/update multiple attributes on the session.
 */
void USessionSubsystem::SetAttributes(const TArray<FSessionAttribute>& Attributes)
{
	// Can only update the session-attributes if owner.
	if(Session.OwnerID != LocalUserSubsystem->GetLocalUser()->GetProductUserID())
	{
		UE_LOG(LogSessionSubsystem, Error, TEXT("Only the session owner can set its attributes."));
		return;
	}

	// Filter out the attributes that have changed and are non-special.
	const TArray<FSessionAttribute> ChangedAttributes = FilterAttributes(Attributes);
	if (!ChangedAttributes.Num()) return;

	// Options for creating the Modification-Handle
	EOS_Sessions_UpdateSessionModificationOptions UpdateSessionModificationOptions;
	UpdateSessionModificationOptions.ApiVersion = EOS_SESSIONS_UPDATESESSIONMODIFICATION_API_LATEST;
	UpdateSessionModificationOptions.SessionName = TCHAR_TO_UTF8(*Session.Name);

	EOS_HSessionModification SessionModificationHandle;
	if (const EOS_EResult Result = EOS_Sessions_UpdateSessionModification(SessionHandle, &UpdateSessionModificationOptions, &SessionModificationHandle); Result == EOS_EResult::EOS_Success)
	{
		// Loop through all attributes and add them to the Handle
		TArray<FSessionAttribute> SuccessfulAttributes;
		for (const FSessionAttribute Attribute : ChangedAttributes)
		{
			if(AddAttributeToHandle(SessionModificationHandle, Attribute)) SuccessfulAttributes.Add(Attribute);
		}
			
		// Update the session with the Handle.
		EOS_Sessions_UpdateSessionOptions UpdateSessionOptions;
		UpdateSessionOptions.ApiVersion = EOS_SESSIONS_UPDATESESSION_API_LATEST;
		UpdateSessionOptions.SessionModificationHandle = SessionModificationHandle;
		
		FSetAttributesClientData* SetAttributeCompleteClientData = new FSetAttributesClientData{this, SuccessfulAttributes};
		EOS_Sessions_UpdateSession(SessionHandle, &UpdateSessionOptions, SetAttributeCompleteClientData, [](const EOS_Sessions_UpdateSessionCallbackInfo* Data)
		{
			const FSetAttributesClientData* ClientData = static_cast<FSetAttributesClientData*>(Data->ClientData);
			
			if(Data->ResultCode == EOS_EResult::EOS_Success)
			{
				// Cache the updated attribute on the session.
				TArray<FSessionAttribute> ChangedAttributes = ClientData->ChangedAttributes;
				for (FSessionAttribute Attribute : ChangedAttributes) ClientData->Self->Session.Attributes.Add(Attribute.Key, Attribute);
				UE_LOG(LogSessionSubsystem, Log, TEXT("Session Attribute(s) successfully added."))
			}
			else UE_LOG(LogSessionSubsystem, Error, TEXT("Failed to update the session with the new attribute(s). Result-Code: [%s]"), *FString(EOS_EResult_ToString(Data->ResultCode)));

			delete ClientData;
		});

		// Release the memory of the Handle.
		EOS_SessionModification_Release(SessionModificationHandle);
	}
	else UE_LOG(LogSessionSubsystem, Error, TEXT("Failed to create the session-modification-handle for setting the attribute(s). Result-Code: [%s]"), *FString(EOS_EResult_ToString(Result)));
}

/**
 * Data needed in EOS_Sessions_UpdateSession after updating a special attribute.
 */
struct FSetSpecialAttributeClientData
{
	USessionSubsystem* Self;
	FSessionAttribute Attribute;
	TFunction<void(bool bWasSuccessful)> Callback;
};

void USessionSubsystem::SetSpecialAttribute(const FSessionAttribute& Attribute, const TFunction<void(bool bWasSuccessful)>& Callback)
{
	// Can only update the session-attributes if owner.
	if(Session.OwnerID != LocalUserSubsystem->GetLocalUser()->GetProductUserID())
	{
		UE_LOG(LogSessionSubsystem, Error, TEXT("Only the session owner can set its attributes."));
		Callback(false);
		return;
	}

	if(!SpecialAttributes.Contains(Attribute.Key))
	{
		UE_LOG(LogSessionSubsystem, Error, TEXT("Custom session-attributes should be set using the ::SetAttributes method."));
		Callback(false);
		return;
	}

	// Options for creating the Modification-Handle
	EOS_Sessions_UpdateSessionModificationOptions UpdateSessionModificationOptions;
	UpdateSessionModificationOptions.ApiVersion = EOS_SESSIONS_UPDATESESSIONMODIFICATION_API_LATEST;
	UpdateSessionModificationOptions.SessionName = TCHAR_TO_UTF8(*Session.Name);

	EOS_HSessionModification SessionModificationHandle;
	if (const EOS_EResult Result = EOS_Sessions_UpdateSessionModification(SessionHandle, &UpdateSessionModificationOptions, &SessionModificationHandle); Result == EOS_EResult::EOS_Success)
	{
		if(!AddAttributeToHandle(SessionModificationHandle, Attribute))
		{
			Callback(false);
			return;
		}
			
		// Update the session with the Handle.
		EOS_Sessions_UpdateSessionOptions UpdateSessionOptions;
		UpdateSessionOptions.ApiVersion = EOS_SESSIONS_UPDATESESSION_API_LATEST;
		UpdateSessionOptions.SessionModificationHandle = SessionModificationHandle;
		
		FSetSpecialAttributeClientData* SetSpecialAttributeCompleteClientData = new FSetSpecialAttributeClientData{this, Attribute, Callback};
		EOS_Sessions_UpdateSession(SessionHandle, &UpdateSessionOptions, SetSpecialAttributeCompleteClientData, [](const EOS_Sessions_UpdateSessionCallbackInfo* Data)
		{
			const FSetSpecialAttributeClientData* ClientData = static_cast<FSetSpecialAttributeClientData*>(Data->ClientData);
			
			if(Data->ResultCode == EOS_EResult::EOS_Success)
			{
				// Cache the updated attribute on the session.
				const FSessionAttribute ChangedAttribute = ClientData->Attribute;
				ClientData->Self->Session.Attributes.Add(ChangedAttribute.Key, ChangedAttribute);
				UE_LOG(LogSessionSubsystem, Log, TEXT("Special session-attribute successfully updated."))
				ClientData->Callback(true);
			}
			else
			{
				UE_LOG(LogSessionSubsystem, Error, TEXT("Failed to update the special session-attribute. Result-Code: [%s]"), *FString(EOS_EResult_ToString(Data->ResultCode)));
				ClientData->Callback(false);
			}

			delete ClientData;
		});

		// Release the memory of the Handle.
		EOS_SessionModification_Release(SessionModificationHandle);
	}
	else
	{
		UE_LOG(LogSessionSubsystem, Error, TEXT("Failed to create the session-modification-handle for setting the special session-attribute. Result-Code: [%s]"), *FString(EOS_EResult_ToString(Result)));
		Callback(false);
	}
}

/*
 * Remember to release the handle after using it with EOS_Session().
 */
EOS_HActiveSession USessionSubsystem::GetActiveSessionHandle() const
{
	EOS_Sessions_CopyActiveSessionHandleOptions Options;
	Options.ApiVersion = EOS_SESSIONS_COPYACTIVESESSIONHANDLE_API_LATEST;
	Options.SessionName = TCHAR_TO_UTF8(*Session.Settings.Name);
	
	EOS_HActiveSession ActiveSessionHandle;
	if(EOS_Sessions_CopyActiveSessionHandle(SessionHandle, &Options, &ActiveSessionHandle) == EOS_EResult::EOS_Success) return ActiveSessionHandle;
	
	UE_LOG(LogSessionSubsystem, Log, TEXT("Failed to get the Active-Session-Handle."));
	return nullptr;
}

void USessionSubsystem::LoadSession(TFunction<void(bool bSuccess)> OnCompleteCallback)
{
	if(!ActiveSession())
	{
		UE_LOG(LogSessionSubsystem, Log, TEXT("Must be in a session first before calling ::LoadSession"));
		OnCompleteCallback(false);
		return;
	}
	if(!SessionDetailsHandle)
	{
		UE_LOG(LogSessionSubsystem, Log, TEXT("SessionDetailsHandle is invalid in ::LoadSession. It should be set after joining a session."));
		OnCompleteCallback(false);
		return;
	}

	constexpr EOS_SessionDetails_CopyInfoOptions CopyInfoOptions {EOS_SESSIONDETAILS_COPYINFO_API_LATEST};
	EOS_SessionDetails_Info* SessionInfo;
	if(const EOS_EResult Result = EOS_SessionDetails_CopyInfo(SessionDetailsHandle, &CopyInfoOptions, &SessionInfo); Result == EOS_EResult::EOS_Success)
	{
		// Store the details we need
		Session.ID = FString(SessionInfo->SessionId);
		
		
		EOS_SessionDetails_Info_Release(SessionInfo);
	}
	else
	{
		EOS_SessionDetails_Release(SessionDetailsHandle);
		OnCompleteCallback(false);
	}
}

// Copyright © 2023 Melvin Brink

#include "Subsystems/User/Local/SteamLocalUserSubsystem.h"
#include "Subsystems/User/Online/SteamOnlineUserSubsystem.h"
#include "Types/UserTypes.h"

#pragma warning(push)
#pragma warning(disable: 4996)
#pragma warning(disable: 4265)
#include "steam_api.h"
#include "isteamuser.h"
#include "steamnetworkingtypes.h"
#pragma warning(pop)



void USteamLocalUserSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	SteamOnlineUserSubsystem = Collection.InitializeDependency<USteamOnlineUserSubsystem>();
}


// --------------------------------------------


/**
 * Requests the encrypted app ticket from Steam.
 *
 * Will call OnEncryptedAppTicketResponse when the request is done.
 * Bind to OnEncryptedAppTicketReady delegate to receive the ticket in your callback.
 */
void USteamLocalUserSubsystem::RequestEncryptedAppTicket()
{
	// TODO: Check if we already have a ticket and if it is still valid.

	// uint8 TicketBuffer[1024];
	// uint32 TicketSize = 0;
	// if(SteamUser()->GetEncryptedAppTicket(TicketBuffer, sizeof(TicketBuffer), &TicketSize))
	// {
	// 	OnEncryptedAppTicketReady.Broadcast(TArray<uint8>(TicketBuffer, TicketSize));
	// 	return;
	// }

	const SteamAPICall_t RequestEncryptedAppTicket = SteamUser()->RequestEncryptedAppTicket(nullptr, 0);
	m_EncryptedAppTicketResponseCallResult.Set(RequestEncryptedAppTicket, this, &USteamLocalUserSubsystem::OnEncryptedAppTicketResponse);
}

/**
 * Called when RequestSteamEncryptedAppTicket is done.
 *
 * Broadcasts OnEncryptedAppTicketReady delegate on success.
 */
void USteamLocalUserSubsystem::OnEncryptedAppTicketResponse(EncryptedAppTicketResponse_t* pEncryptedAppTicketResponse, bool bIOFailure)
{
	if (bIOFailure)
	{
		UE_LOG(LogSteamLocalUserSubsystem, Error, TEXT("There has been an IO Failure when requesting the Encrypted App Ticket."));
		OnEncryptedAppTicketReady.Broadcast(TArray<uint8>());
		return;
	}
	
	if (pEncryptedAppTicketResponse->m_eResult == k_EResultOK)
	{
		uint8 TicketBuffer[1024];
		uint32 TicketSize = 0;
		if(SteamUser()->GetEncryptedAppTicket(TicketBuffer, sizeof(TicketBuffer), &TicketSize))
		{
			OnEncryptedAppTicketReady.Broadcast(TArray<uint8>(TicketBuffer, TicketSize));
			return;
		}
		
		UE_LOG(LogSteamLocalUserSubsystem, Error, TEXT("GetEncryptedAppTicket failed."));
		return;
	}

	// Log errors.
	switch (pEncryptedAppTicketResponse->m_eResult) {
	case k_EResultNoConnection:
		UE_LOG(LogSteamLocalUserSubsystem, Error, TEXT("Calling RequestEncryptedAppTicket while not connected to Steam results in this error."));
		break;
	case k_EResultDuplicateRequest:
		UE_LOG(LogSteamLocalUserSubsystem, Error, TEXT("Calling RequestEncryptedAppTicket while there is already a pending request results in this error."));
		break;
	case k_EResultLimitExceeded:
		UE_LOG(LogSteamLocalUserSubsystem, Error, TEXT("Calling RequestEncryptedAppTicket more than once per minute returns this error."));
		break;
	default:
		UE_LOG(LogSteamLocalUserSubsystem, Error, TEXT("Encrypted App Ticket response returned an unexpected result: %d"), static_cast<int32>(pEncryptedAppTicketResponse->m_eResult));
		break;
	}
	
	OnEncryptedAppTicketReady.Broadcast(TArray<uint8>());
}


// --------------------------------


/**
 * Will request the session ticket from Steam.
 *
 * OnSessionTicketResponse will be called when the request is done.
 *
 * Broadcasts OnSessionTicketReady delegate when the ticket is already valid. This will not call OnSessionTicketResponse.
 */
void USteamLocalUserSubsystem::RequestSessionTicket()
{
	if(SessionTicket.Num())
	{
		// If the session ticket is ready/validated, don't request a new one.
		if(bSessionTicketReady)
		{
			UE_LOG(LogSteamLocalUserSubsystem, Log, TEXT("Session-Ticket already validated."));
			OnSessionTicketReady.Broadcast(SessionTicket);
			return;
		}

		// If we are already waiting for a response, don't request a new one.
		if(bWaitingForSessionTicket)
		{
			UE_LOG(LogSteamLocalUserSubsystem, Log, TEXT("Session-Ticket already requested. Waiting for response..."));
			return;
		}
	}

	bSessionTicketReady = false;
	bWaitingForSessionTicket = true;
	
	uint8 TicketBuffer[1024];
	uint32 TicketSize = 0;
	Identity.SetSteamID(SteamUser()->GetSteamID());
	const HAuthTicket AuthTicket = SteamUser()->GetAuthSessionTicket(TicketBuffer, sizeof(TicketBuffer), &TicketSize, &Identity);

	if (AuthTicket == k_HAuthTicketInvalid)
	{
		UE_LOG(LogSteamLocalUserSubsystem, Error, TEXT("Failed to get Session-Ticket."));
		bWaitingForSessionTicket = false;
		OnSessionTicketReady.Broadcast(TArray<uint8>());
		return;
	}

	SessionTicket = TArray<uint8>(TicketBuffer, TicketSize);
}

/**
 * Called on response from Steam after calling GetAuthSessionTicket.
 *
 * Broadcasts OnSessionTicketReady delegate on success.
 */
void USteamLocalUserSubsystem::OnSessionTicketResponse(GetAuthSessionTicketResponse_t* Data)
{
	if (Data->m_eResult == k_EResultOK)
	{
		bSessionTicketReady = true;
		OnSessionTicketReady.Broadcast(SessionTicket);
	}
	else
	{
		UE_LOG(LogSteamLocalUserSubsystem, Error, TEXT("OnSessionTicketResponse: failed, error code: [%d]."), Data->m_eResult);
		OnSessionTicketReady.Broadcast(TArray<uint8>());
	}
}


// --------------------------------


void USteamLocalUserSubsystem::LoadLocalUserDetails(ULocalUser& LocalUser)
{
	const uint64 UserID = SteamUser()->GetSteamID().ConvertToUint64();
	LocalUser.SetUserID(UserID);
	LocalUser.SetUsername(FString(UTF8_TO_TCHAR(SteamFriends()->GetPersonaName())));

	SteamOnlineUserSubsystem->FetchAvatar(UserID, [&LocalUser](UTexture2D* Avatar)
	{
		LocalUser.SetAvatar(Avatar);
	});
}

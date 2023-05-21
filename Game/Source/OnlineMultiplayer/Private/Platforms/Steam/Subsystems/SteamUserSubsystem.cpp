// Copyright © 2023 Melvin Brink

#include "Platforms/Steam/Subsystems/SteamUserSubsystem.h"

#pragma warning(push)
#pragma warning(disable: 4996)
#pragma warning(disable: 4265)
#include "steam_api.h"
#include "isteamuser.h"
#include "steamnetworkingtypes.h"
#pragma warning(pop)

#include <string>



void USteamUserSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}


// --------------------------------------------


/**
 * Requests the encrypted app ticket from Steam.
 *
 * Will call OnEncryptedAppTicketResponse when the request is done.
 * Bind to OnEncryptedAppTicketReady delegate to receive the ticket in your callback.
 */
void USteamUserSubsystem::RequestEncryptedAppTicket()
{
	CHECK_STEAM

	// TODO: Check if we already have a ticket and if it is still valid.

	// uint8 TicketBuffer[1024];
	// uint32 TicketSize = 0;
	// if(SteamUser()->GetEncryptedAppTicket(TicketBuffer, sizeof(TicketBuffer), &TicketSize))
	// {
	// 	OnEncryptedAppTicketReady.Broadcast(TArray<uint8>(TicketBuffer, TicketSize));
	// 	return;
	// }

	const SteamAPICall_t RequestEncryptedAppTicket = SteamUser()->RequestEncryptedAppTicket(nullptr, 0);
	m_EncryptedAppTicketResponseCallResult.Set(RequestEncryptedAppTicket, this, &USteamUserSubsystem::OnEncryptedAppTicketResponse);
}

/**
 * Called when RequestSteamEncryptedAppTicket is done.
 *
 * Broadcasts OnEncryptedAppTicketReady delegate on success.
 */
void USteamUserSubsystem::OnEncryptedAppTicketResponse(EncryptedAppTicketResponse_t* pEncryptedAppTicketResponse, bool bIOFailure)
{
	if (bIOFailure)
	{
		UE_LOG(LogSteamUserSubsystem, Error, TEXT("There has been an IO Failure when requesting the Encrypted App Ticket."));
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
		
		UE_LOG(LogSteamUserSubsystem, Error, TEXT("GetEncryptedAppTicket failed."));
		return;
	}

	// Log errors.
	switch (pEncryptedAppTicketResponse->m_eResult) {
	case k_EResultNoConnection:
		UE_LOG(LogSteamUserSubsystem, Error, TEXT("Calling RequestEncryptedAppTicket while not connected to Steam results in this error."));
		break;
	case k_EResultDuplicateRequest:
		UE_LOG(LogSteamUserSubsystem, Error, TEXT("Calling RequestEncryptedAppTicket while there is already a pending request results in this error."));
		break;
	case k_EResultLimitExceeded:
		UE_LOG(LogSteamUserSubsystem, Error, TEXT("Calling RequestEncryptedAppTicket more than once per minute returns this error."));
		break;
	default:
		UE_LOG(LogSteamUserSubsystem, Error, TEXT("Encrypted App Ticket response returned an unexpected result: %d"), static_cast<int32>(pEncryptedAppTicketResponse->m_eResult));
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
void USteamUserSubsystem::RequestSessionTicket()
{
	CHECK_STEAM
	
	if(SessionTicket.Num())
	{
		// If the session ticket is ready/validated, don't request a new one.
		if(bSessionTicketReady)
		{
			UE_LOG(LogSteamUserSubsystem, Log, TEXT("Session-Ticket already validated."));
			OnSessionTicketReady.Broadcast(SessionTicket);
			return;
		}

		// If we are already waiting for a response, don't request a new one.
		if(bWaitingForSessionTicket)
		{
			UE_LOG(LogSteamUserSubsystem, Log, TEXT("Session-Ticket already requested. Waiting for response..."));
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
		UE_LOG(LogSteamUserSubsystem, Error, TEXT("Failed to get Session-Ticket."));
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
void USteamUserSubsystem::OnSessionTicketResponse(GetAuthSessionTicketResponse_t* Data)
{
	if (Data->m_eResult == k_EResultOK)
	{
		bSessionTicketReady = true;
		OnSessionTicketReady.Broadcast(SessionTicket);
	}
	else
	{
		UE_LOG(LogSteamUserSubsystem, Error, TEXT("OnSessionTicketResponse: failed, error code: [%d]."), Data->m_eResult);
		OnSessionTicketReady.Broadcast(TArray<uint8>());
	}
}


// --------------------------------


void USteamUserSubsystem::GetUserAvatar(const std::string UserIDString)
{
	CHECK_STEAM
	const CSteamID UserID(std::stoull(UserIDString));

	// Return if the image is already being requested. Callback will be called when the image is ready.
	if (SteamFriends()->RequestUserInformation(UserID, false)) return;

	// Image is ready.
	ProcessAvatar(UserID);
}

void USteamUserSubsystem::ProcessAvatar(const CSteamID& UserID)
{
	// Listen to the callback when the image is not yet ready
	if (const int ImageData = SteamFriends()->GetSmallFriendAvatar(UserID); ImageData != -1) 
	{
		uint32 ImageWidth, ImageHeight;
		if (SteamUtils()->GetImageSize(ImageData, &ImageWidth, &ImageHeight)) 
		{
			// Create buffer large enough to hold the image data
			std::vector<uint8> Buffer(ImageWidth * ImageHeight * 4); // 4 bytes per pixel for RGBA

			// Get the actual image data
			if (SteamUtils()->GetImageRGBA(ImageData, Buffer.data(), Buffer.size())) 
			{
				// Avatar image data is now in the buffer
				// You can use this data to create a texture, save it to a file, etc.
			}
		}
	}
}

void USteamUserSubsystem::OnPersonaStateChange(PersonaStateChange_t* Data)
{
	CHECK_STEAM
	const CSteamID UserID(Data->m_ulSteamID);

	// If avatar data changed, process it again.
	if (Data->m_nChangeFlags & k_EPersonaChangeAvatar)
	{
		ProcessAvatar(UserID);
	}
}
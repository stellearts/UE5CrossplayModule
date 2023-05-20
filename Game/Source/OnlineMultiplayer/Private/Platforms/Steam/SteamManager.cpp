// Copyright © 2023 Melvin Brink

#include "Platforms/Steam/SteamManager.h"
#pragma warning(push)
#pragma warning(disable: 4996)
#pragma warning(disable: 4265)
#include <string>

#include "steam_api.h"
#include "isteamuser.h"
#include "steamnetworkingtypes.h"
#pragma warning(pop)



void SteamAPIDebugMessageHook(int nSeverity, const char *pchDebugText)
{
	// Output the debug message to your desired logging method
	printf("SteamAPI Debug: %s\n", pchDebugText);
}

FSteamManager::FSteamManager()
{
	
}

void FSteamManager::Tick(float DeltaTime)
{
	SteamAPI_RunCallbacks();
}

bool FSteamManager::IsTickable() const
{
	return true;
}

TStatId FSteamManager::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FSteamManager, STATGROUP_Tickables);
}

FSteamManager& FSteamManager::Get()
{
	static FSteamManager Instance;
	return Instance;
}


// --------------------------------


void FSteamManager::Initialize()
{
	UE_LOG(LogSteamManager, Log, TEXT("Initializing Steam-Subsystem"));
	
	if (SteamAPI_RestartAppIfNecessary(2428930))
	{
		// The app is being restarted to use the specified AppID; exit gracefully
		FPlatformMisc::RequestExit(false);
		return;
	}
	
	if (SteamAPI_Init())
	{
		UE_LOG(LogSteamManager, Log, TEXT("Steamworks SDK Initialized"));
		SteamUtils()->SetWarningMessageHook(SteamAPIDebugMessageHook);
	}
	else
	{
		UE_LOG(LogSteamManager, Error, TEXT("Failed to initialize Steamworks SDK"));
	}
}

void FSteamManager::DeInitialize()
{
	if (SteamAPI_Init()) SteamAPI_Shutdown();
}


// --------------------------------


/**
 * Requests the encrypted app ticket from Steam.
 *
 * Will call OnEncryptedAppTicketResponse when the request is done.
 * Bind to OnEncryptedAppTicketReady delegate to receive the ticket in your callback.
 */
void FSteamManager::RequestEncryptedAppTicket()
{
	if (!SteamAPI_Init())
	{
		LOG_STEAM_NULL
		return;
	}

	// TODO: Check if we already have a ticket and if it is still valid.

	// uint8 TicketBuffer[1024];
	// uint32 TicketSize = 0;
	// if(SteamUser()->GetEncryptedAppTicket(TicketBuffer, sizeof(TicketBuffer), &TicketSize))
	// {
	// 	OnEncryptedAppTicketReady.Broadcast(TArray<uint8>(TicketBuffer, TicketSize));
	// 	return;
	// }

	const SteamAPICall_t RequestEncryptedAppTicket = SteamUser()->RequestEncryptedAppTicket(nullptr, 0);
	m_EncryptedAppTicketResponseCallResult.Set(RequestEncryptedAppTicket, this, &FSteamManager::OnEncryptedAppTicketResponse);
}

/**
 * Called when RequestSteamEncryptedAppTicket is done.
 *
 * Broadcasts OnEncryptedAppTicketReady delegate on success.
 */
void FSteamManager::OnEncryptedAppTicketResponse(EncryptedAppTicketResponse_t* pEncryptedAppTicketResponse, bool bIOFailure) {
	if (bIOFailure)
	{
		UE_LOG(LogSteamManager, Error, TEXT("There has been an IO Failure when requesting the Encrypted App Ticket."));
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
		
		UE_LOG(LogSteamManager, Error, TEXT("GetEncryptedAppTicket failed."));
		return;
	}

	// Log errors.
	switch (pEncryptedAppTicketResponse->m_eResult) {
	case k_EResultNoConnection:
		UE_LOG(LogSteamManager, Error, TEXT("Calling RequestEncryptedAppTicket while not connected to Steam results in this error."));
		break;
	case k_EResultDuplicateRequest:
		UE_LOG(LogSteamManager, Error, TEXT("Calling RequestEncryptedAppTicket while there is already a pending request results in this error."));
		break;
	case k_EResultLimitExceeded:
		UE_LOG(LogSteamManager, Error, TEXT("Calling RequestEncryptedAppTicket more than once per minute returns this error."));
		break;
	default:
		UE_LOG(LogSteamManager, Error, TEXT("Encrypted App Ticket response returned an unexpected result: %d"), static_cast<int32>(pEncryptedAppTicketResponse->m_eResult));
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
void FSteamManager::RequestSessionTicket() {
	if (!SteamAPI_Init())
	{
		LOG_STEAM_NULL
		return;
	}
	
	if(SessionTicket.Num())
	{
		// If the session ticket is ready/validated, don't request a new one.
		if(bSessionTicketReady)
		{
			UE_LOG(LogSteamManager, Log, TEXT("Session-Ticket already validated."));
			OnSessionTicketReady.Broadcast(SessionTicket);
			return;
		}

		// If we are already waiting for a response, don't request a new one.
		if(bWaitingForSessionTicket)
		{
			UE_LOG(LogSteamManager, Log, TEXT("Session-Ticket already requested. Waiting for response..."));
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
		UE_LOG(LogSteamManager, Error, TEXT("Failed to get Session-Ticket."));
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
void FSteamManager::OnSessionTicketResponse(GetAuthSessionTicketResponse_t* Data)
{
	if (Data->m_eResult == k_EResultOK)
	{
		bSessionTicketReady = true;
		OnSessionTicketReady.Broadcast(SessionTicket);
	}
	else
	{
		UE_LOG(LogSteamManager, Error, TEXT("OnSessionTicketResponse: failed, error code: [%d]."), Data->m_eResult);
		OnSessionTicketReady.Broadcast(TArray<uint8>());
	}
}


// --------------------------------


void FSteamManager::GetUserAvatar(const std::string UserIDString)
{
	const CSteamID UserID(std::stoull(UserIDString));

	// Return if the image is already being requested. Callback will be called when the image is ready.
	if (SteamFriends()->RequestUserInformation(UserID, false)) return;

	// Image is ready.
	ProcessAvatar(UserID);
}

void FSteamManager::ProcessAvatar(const CSteamID& UserID)
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

void FSteamManager::OnPersonaStateChange(PersonaStateChange_t* Data)
{
	const CSteamID UserID(Data->m_ulSteamID);

	// If avatar data changed, try to process the avatar again
	if (Data->m_nChangeFlags & k_EPersonaChangeAvatar)
	{
		ProcessAvatar(UserID);
	}
}
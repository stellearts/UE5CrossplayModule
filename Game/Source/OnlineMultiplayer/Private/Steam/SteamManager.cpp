// Copyright © 2023 Melvin Brink

#include "Steam/SteamManager.h"

#include "steam_api.h"
#include "isteamuser.h"
#include "steamnetworkingtypes.h"



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

	// Steam DevAppID to 480 for testing.
	// if (SteamAPI_RestartAppIfNecessary(480))
	// {
	// 	// The app is being restarted to use the specified AppID; exit gracefully
	// 	FPlatformMisc::RequestExit(false);
	// 	return;
	// }
	
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
 * Will request an encrypted app ticket from Steam.
 *
 * Calls OnEncryptedAppTicketResponse in this class when the request is done.
 *
 * This should only be called once, at the start of the game.
 */
void FSteamManager::RequestSteamEncryptedAppTicket() {
	if (!SteamAPI_Init())
	{
		LOG_STEAM_NULL
		return;
	}
	UE_LOG(LogSteamManager, Log, TEXT("Requesting Encrypted App Ticket"));
	SteamAPICall_t hSteamAPICall = SteamUser()->RequestEncryptedAppTicket(nullptr, 0);
	m_EncryptedAppTicketResponse.Set(hSteamAPICall, this, &FSteamManager::OnEncryptedAppTicketResponse);
}

/**
 * Called when RequestSteamEncryptedAppTicket is done.
 *
 * Broadcasts OnEncryptedAppTicketReady delegate on success.
 */
void FSteamManager::OnEncryptedAppTicketResponse(EncryptedAppTicketResponse_t* pEncryptedAppTicketResponse, bool bIOFailure) {
	UE_LOG(LogSteamManager, Warning, TEXT("OnEncryptedAppTicketResponse"));
	if (bIOFailure) {
		UE_LOG(LogSteamManager, Error, TEXT("There has been an IO Failure when requesting the Encrypted App Ticket."));
		return;
	}

	switch (pEncryptedAppTicketResponse->m_eResult) {
	case k_EResultOK:
		UE_LOG(LogSteamManager, Warning, TEXT("Broadcasting OnEncryptedAppTicketReady"));
		OnEncryptedAppTicketReady.Broadcast();
		break;
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
}

/**
 * Gets the encrypted app ticket from Steam.
 *
 * This should only be called after OnEncryptedAppTicketReady has been broad-casted.
 *
 * @returns ByteArray containing the encrypted app ticket.
 */
TArray<uint8> FSteamManager::GetEncryptedAppTicket()
{
	if (!SteamAPI_Init())
	{
		LOG_STEAM_NULL
		return TArray<uint8>();
	}
	
	uint8 TicketBuffer[1024];
	uint32 TicketSize;
	if (SteamUser()->GetEncryptedAppTicket(TicketBuffer, sizeof(TicketBuffer), &TicketSize))
	{
		return TArray<uint8>(TicketBuffer, TicketSize);
	}

	UE_LOG(LogSteamManager, Error, TEXT("Failed to get encrypted app ticket"));
	return TArray<uint8>();
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
			UE_LOG(LogSteamManager, Log, TEXT("Session ticket already validated."));
			OnSessionTicketReady.Broadcast(SessionTicket);
			return;
		}

		// If we are already waiting for a session response, don't request a new one.
		if(bWaitingForSessionTicket)
		{
			UE_LOG(LogSteamManager, Log, TEXT("Session ticket already requested. Waiting for response..."));
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
		UE_LOG(LogSteamManager, Error, TEXT("Failed to get Auth Session Ticket."));
		bWaitingForSessionTicket = false;
		return;
	}

	SessionTicket = TArray<uint8>(TicketBuffer, TicketSize);
}

/**
 * Called on response from Steam after calling GetAuthSessionTicket.
 *
 * Broadcasts OnSessionTicketReady delegate on success.
 */
void FSteamManager::OnSessionTicketResponse(GetAuthSessionTicketResponse_t *Result)
{
	if (Result->m_eResult == k_EResultOK)
	{
		UE_LOG(LogSteamManager, Log, TEXT("OnSessionTicketResponse: success, got ticket."));
		bSessionTicketReady = true;
		OnSessionTicketReady.Broadcast(SessionTicket);
	}
	else
	{
		UE_LOG(LogSteamManager, Error, TEXT("OnSessionTicketResponse: failed, error code: [%d]."), Result->m_eResult);
		OnSessionTicketReady.Broadcast(TArray<uint8>());
	}
}
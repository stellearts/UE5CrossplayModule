// Copyright © 2023 Melvin Brink

#include "Online/SteamworksSubsystem.h"

#include "Steam/steam_api.h"
#include "Steam/isteamuser.h"
#include "Steam/steamnetworkingtypes.h"



void SteamAPIDebugMessageHook(int nSeverity, const char *pchDebugText)
{
	// Output the debug message to your desired logging method
	printf("SteamAPI Debug: %s\n", pchDebugText);
}

void USteamworksSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogSteamSubsystem, Log, TEXT("Initializing Steam-Subsystem"));
	
	InitializeSteamworks();
}

void USteamworksSubsystem::Deinitialize()
{
	SteamAPI_Shutdown();
	
	Super::Deinitialize();
}

void USteamworksSubsystem::InitializeSteamworks()
{
	// Steam DevAppID to 480 for testing.
	if (SteamAPI_RestartAppIfNecessary(480))
	{
		// The app is being restarted to use the specified AppID; exit gracefully
		FPlatformMisc::RequestExit(false);
		return;
	}
	
	if (SteamAPI_Init())
	{
		UE_LOG(LogSteamSubsystem, Log, TEXT("Steamworks SDK Initialized"));
		SteamUtils()->SetWarningMessageHook(SteamAPIDebugMessageHook);
	}
	else
	{
		UE_LOG(LogSteamSubsystem, Error, TEXT("Failed to initialize Steamworks SDK"));
	}
}


// --------------------------------


/**
 * Will request an encrypted app ticket from Steam.
 *
 * Calls OnEncryptedAppTicketResponse in this class when the request is done.
 *
 * This should only be called once, at the start of the game.
 */
void USteamworksSubsystem::RequestSteamEncryptedAppTicket() {
	if (!SteamAPI_Init())
	{
		LOG_STEAM_NULL
		return;
	}
	UE_LOG(LogSteamSubsystem, Log, TEXT("Requesting Encrypted App Ticket"));
	SteamAPICall_t hSteamAPICall = SteamUser()->RequestEncryptedAppTicket(nullptr, 0);
	m_EncryptedAppTicketResponse.Set(hSteamAPICall, this, &ThisClass::OnEncryptedAppTicketResponse);
}

/**
 * Called when RequestSteamEncryptedAppTicket is done.
 *
 * Broadcasts OnEncryptedAppTicketReady delegate on success.
 */
void USteamworksSubsystem::OnEncryptedAppTicketResponse(EncryptedAppTicketResponse_t* pEncryptedAppTicketResponse, bool bIOFailure) {
	UE_LOG(LogSteamSubsystem, Warning, TEXT("OnEncryptedAppTicketResponse"));
	if (bIOFailure) {
		UE_LOG(LogSteamSubsystem, Error, TEXT("There has been an IO Failure when requesting the Encrypted App Ticket."));
		return;
	}

	switch (pEncryptedAppTicketResponse->m_eResult) {
	case k_EResultOK:
		UE_LOG(LogSteamSubsystem, Warning, TEXT("Broadcasting OnEncryptedAppTicketReady"));
		OnEncryptedAppTicketReady.Broadcast();
		break;
	case k_EResultNoConnection:
		UE_LOG(LogSteamSubsystem, Error, TEXT("Calling RequestEncryptedAppTicket while not connected to Steam results in this error."));
		break;
	case k_EResultDuplicateRequest:
		UE_LOG(LogSteamSubsystem, Error, TEXT("Calling RequestEncryptedAppTicket while there is already a pending request results in this error."));
		break;
	case k_EResultLimitExceeded:
		UE_LOG(LogSteamSubsystem, Error, TEXT("Calling RequestEncryptedAppTicket more than once per minute returns this error."));
		break;
	default:
		UE_LOG(LogSteamSubsystem, Error, TEXT("Encrypted App Ticket response returned an unexpected result: %d"), static_cast<int32>(pEncryptedAppTicketResponse->m_eResult));
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
TArray<uint8> USteamworksSubsystem::GetEncryptedAppTicket()
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

	UE_LOG(LogSteamSubsystem, Error, TEXT("Failed to get encrypted app ticket"));
	return TArray<uint8>();
}


// --------------------------------


/**
 * Will request an encrypted app ticket from Steam.
 *
 * Calls OnEncryptedAppTicketResponse in this class when the request is done.
 *
 * This should only be called once, at the start of the game.
 */
TArray<uint8> USteamworksSubsystem::GetAuthSessionTicket() {
	if (!SteamAPI_Init())
	{
		LOG_STEAM_NULL
		return TArray<uint8>();
	}
	
	uint8 TicketBuffer[1024];
	uint32 TicketSize = 0;
	Identity.SetSteamID(SteamUser()->GetSteamID());
	const HAuthTicket AuthTicket = SteamUser()->GetAuthSessionTicket(TicketBuffer, sizeof(TicketBuffer), &TicketSize, &Identity);
	
	if (AuthTicket == k_HAuthTicketInvalid)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get Auth Session Ticket."));
		return TArray<uint8>();
	}

	return TArray<uint8>(TicketBuffer, TicketSize);
}
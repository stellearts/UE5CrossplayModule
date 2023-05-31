// Copyright © 2023 Melvin Brink

#include "Platforms/EOS/Subsystems/LocalUserSubsystem.h"
#include "Platforms/EOS/EOSManager.h"
#include "Platforms/Steam/SteamManager.h"
#include "Platforms/Steam/Subsystems/SteamUserSubsystem.h"


ULocalUserSubsystem::ULocalUserSubsystem()
{
	EosManager = FEosManager::Get();
	LocalUser = MakeUnique<FLocalUser>();
}

void ULocalUserSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	const EOS_HPlatform PlatformHandle = EosManager.GetPlatformHandle();
	if(!PlatformHandle) return;
	// TODO: EOS user interface or something if it exists at all.


	
	// TODO: Steam should not run at all if playing on different platform, find a way to disable it and run this conditionally.
	// Get the SteamManager and set necessary callbacks.
	SteamUserSubsystem = Collection.InitializeDependency<USteamUserSubsystem>();
	SteamUserSubsystem->OnSessionTicketReady.AddUObject(this, &ULocalUserSubsystem::OnSteamSessionTicketResponse);
	SteamUserSubsystem->OnEncryptedAppTicketReady.AddUObject(this, &ULocalUserSubsystem::OnSteamEncryptedAppTicketResponse);
}


// --------------------------------------------


/**
 * Requests an encrypted app ticket from Steam.
 *
 * @param TicketReadyCallback The callback to call when the ticket is ready.
 */
void ULocalUserSubsystem::RequestSteamEncryptedAppTicket(const TFunction<void(std::string Ticket)> TicketReadyCallback)
{
	SteamEncryptedAppTicketCallback = TicketReadyCallback;
	SteamUserSubsystem->RequestEncryptedAppTicket();
}

/**
 * Called when the Steam encrypted app ticket is ready.
 *
 * Converts the ticket to a string and broadcasts the delegate.
 */
void ULocalUserSubsystem::OnSteamEncryptedAppTicketResponse(const TArray<uint8> Ticket)
{
	if (Ticket.Num() > 0)
	{
		char Buffer[1024] = "";
		uint32_t Len = 1024;
		if (const EOS_EResult Result = EOS_ByteArray_ToString(Ticket.GetData(), Ticket.Num(), Buffer, &Len); Result != EOS_EResult::EOS_Success)
		{
			UE_LOG(LogLocalUserSubsystem, Error, TEXT("Failed to convert encrypted app ticket to string"));
		}
	
		SteamEncryptedAppTicketCallback(Buffer);
	}
	else
	{
		UE_LOG(LogLocalUserSubsystem, Error, TEXT("Failed to get Auth Session Ticket from Steam"));
		SteamEncryptedAppTicketCallback("");
	}

	// Clear callback
	SteamEncryptedAppTicketCallback = TFunction<void(std::string TicketString)>();
}

/**
 * Requests a session ticket from Steam.
 *
 * @param TicketReadyCallback The callback to call when the ticket is ready.
 */
void ULocalUserSubsystem::RequestSteamSessionTicket(const TFunction<void(std::string TicketString)> TicketReadyCallback)
{
	SteamSessionTicketCallback = TicketReadyCallback;
	SteamUserSubsystem->RequestSessionTicket();
}

/**
 * Called when the Steam session ticket is ready.
 *
 * Converts the ticket to a string and broadcasts the delegate.
 */
void ULocalUserSubsystem::OnSteamSessionTicketResponse(const TArray<uint8> Ticket)
{
	if (Ticket.Num() > 0)
	{
		char Buffer[1024] = "";
		uint32_t Len = 1024;
		if (const EOS_EResult Result = EOS_ByteArray_ToString(Ticket.GetData(), Ticket.Num(), Buffer, &Len); Result != EOS_EResult::EOS_Success)
		{
			UE_LOG(LogLocalUserSubsystem, Error, TEXT("Failed to convert encrypted app ticket to string"));
		}
	
		SteamSessionTicketCallback(Buffer);
	}
	else
	{
		UE_LOG(LogLocalUserSubsystem, Error, TEXT("Failed to get Auth Session Ticket from Steam"));
		SteamSessionTicketCallback("");
	}

	// Clear callback
	SteamSessionTicketCallback = TFunction<void(std::string TicketString)>();
}
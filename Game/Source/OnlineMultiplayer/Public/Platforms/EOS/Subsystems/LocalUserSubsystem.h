// Copyright © 2023 Melvin Brink

#pragma once

#include <string>
#include "CoreMinimal.h"
#include "Platforms/EOS/UserTypes.h"
#include "eos_sdk.h"
#include "LocalUserSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogLocalUserSubsystem, Log, All);
inline DEFINE_LOG_CATEGORY(LogLocalUserSubsystem);



/**
 * Subsystem for managing the user.
 */
UCLASS(BlueprintType)
class ONLINEMULTIPLAYER_API ULocalUserSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	ULocalUserSubsystem();

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

private:
	class FSteamManager* SteamManager;
	class FEosManager* EosManager;
	UPROPERTY() class USteamUserSubsystem* SteamUserSubsystem;
	UPROPERTY() ULocalUser* LocalUser;

public:
	FORCEINLINE ULocalUser* GetLocalUser() const { return LocalUser; }


	
	// ------------------------------- Local User Utilities -------------------------------
	
	void RequestSteamEncryptedAppTicket(const TFunction<void(std::string Ticket)> TicketReadyCallback);
	void RequestSteamSessionTicket(const TFunction<void(std::string Ticket)> TicketReadyCallback);

private:
	void OnSteamEncryptedAppTicketResponse(const TArray<uint8> Ticket);
	TFunction<void(std::string TicketString)> SteamEncryptedAppTicketCallback;
	void OnSteamSessionTicketResponse(const TArray<uint8> Ticket);
	TFunction<void(std::string TicketString)> SteamSessionTicketCallback;
};

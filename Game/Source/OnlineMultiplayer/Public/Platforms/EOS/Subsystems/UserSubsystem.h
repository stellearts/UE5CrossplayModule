// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "eos_sdk.h"
#include <string>
#include "UserSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogUserSubsystem, Log, All);
inline DEFINE_LOG_CATEGORY(LogUserSubsystem);



/**
 * Subsystem for managing the user.
 */
UCLASS(BlueprintType)
class ONLINEMULTIPLAYER_API UUserSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	UUserSubsystem();

	
protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

public:
	void RequestSteamEncryptedAppTicket(const TFunction<void(std::string Ticket)> TicketReadyCallback);
	void RequestSteamSessionTicket(const TFunction<void(std::string Ticket)> TicketReadyCallback);

private:
	void OnSteamEncryptedAppTicketResponse(const TArray<uint8> Ticket);
	TFunction<void(std::string TicketString)> SteamEncryptedAppTicketCallback;
	void OnSteamSessionTicketResponse(const TArray<uint8> Ticket);
	TFunction<void(std::string TicketString)> SteamSessionTicketCallback;
	
	class FSteamManager& SteamManager;
	class FEosManager& EosManager;
	
	UPROPERTY()
	class USteamUserSubsystem* SteamUserSubsystem;
};

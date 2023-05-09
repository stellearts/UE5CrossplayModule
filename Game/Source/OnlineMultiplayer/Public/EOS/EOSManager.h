// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "eos_sdk.h"
#include "Steam/SteamManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogEOSSubsystem, Log, All);
inline DEFINE_LOG_CATEGORY(LogEOSSubsystem);



/**
 * Struct that holds information about a local user.
 *
 * Used to keep track of the local user's state.
 */
struct FLocalUser
{
	bool bConnectLoggedIn = false;
	bool bAuthLoggedIn = false;
	EOS_EpicAccountId EpicAccountID;
	EOS_ProductUserId ProductUserId;
	EOS_ContinuanceToken ContinuanceToken;
};


/**
 * Singleton class for the EOS-SDK.
 * 
 * Responsible for initializing the SDK and providing helper functions and variables for other classes.
 * 
 * Should be used by the subsystems than handle online related functionality.
 */
class ONLINEMULTIPLAYER_API FEosManager final : public FTickableGameObject
{
	// Private constructor to prevent direct instantiation
	FEosManager();
	// Prevent copy-construction
	FEosManager(const FEosManager&) = delete;
	// Prevent assignment
	FEosManager& operator=(const FEosManager&) = delete;
	
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
	
public:
	static FEosManager& Get();
	void Initialize();

private:
	void InitializeSdk();
	void InitializePlatform();
	EOS_EResult CreateIntegratedPlatform(EOS_Platform_Options& PlatformOptions);
	void FreeIntegratedPlatform(EOS_Platform_Options& PlatformOptions);

public:
	void RequestSteamEncryptedAppTicket(const TFunction<void(std::string Ticket)> TicketReadyCallback);
	void RequestSteamSessionTicket(const TFunction<void(std::string Ticket)> TicketReadyCallback);

private:
	static void OnSteamEncryptedAppTicketResponse(const TArray<uint8> Ticket);
	TFunction<void(std::string TicketString)> SteamEncryptedAppTicketCallback;
	static void OnSteamSessionTicketResponse(const TArray<uint8> Ticket);
	TFunction<void(std::string TicketString)> SteamSessionTicketCallback;
	
	FSteamManager* SteamManager;
	EOS_HPlatform PlatformHandle;
	bool bIsInitialized = false;
	FLocalUser LocalUser;

public:
	FORCEINLINE EOS_HPlatform GetPlatformHandle() const { return PlatformHandle; }
	FORCEINLINE FLocalUser& GetLocalUser() { return LocalUser; }
	FORCEINLINE CSteamID GetSteamID() const { return SteamManager->GetSteamID(); }
};

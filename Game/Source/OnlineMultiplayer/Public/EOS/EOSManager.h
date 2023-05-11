// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "eos_sdk.h"
#include "Steam/SteamManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogEOSSubsystem, Log, All);
inline DEFINE_LOG_CATEGORY(LogEOSSubsystem);



/**
 * Enum for the different supported platforms.
 */
enum class EPlatformType : uint8
{
	PlatformType_Steam,
	PlatformType_Psn,
	PlatformType_Xbox,
	PlatformType_Epic,
	PlatformType_None
};

/**
 * Used to keep track of the local user's state.
 */
class FLocalUserState
{
	/**
	 * Struct that holds information about a local user.
	 */
	struct FLocalUser
	{
		EOS_LobbyId LobbyID;
		uint64 ShadowLobbyID = 0;
		EOS_EpicAccountId EpicAccountID;
		EOS_ProductUserId ProductUserId;
		EOS_ContinuanceToken ContinuanceToken;
		EPlatformType PlatformType = EPlatformType::PlatformType_None;
	};
	
	FLocalUser LocalUser;
	
public:
	FLocalUserState() = default;
	
	FORCEINLINE EOS_LobbyId GetLobbyID() const { return LocalUser.LobbyID; }
	FORCEINLINE uint64 GetShadowLobbyID() const { return LocalUser.ShadowLobbyID; }
	FORCEINLINE EOS_EpicAccountId GetEpicAccountId() const { return LocalUser.EpicAccountID; }
	FORCEINLINE EOS_ProductUserId GetProductUserId() const { return LocalUser.ProductUserId; }
	FORCEINLINE EOS_ContinuanceToken GetContinuanceToken() const { return LocalUser.ContinuanceToken; }
	FORCEINLINE EPlatformType GetPlatformType() const { return LocalUser.PlatformType; }
	
	void SetLobbyID(const EOS_LobbyId InLobbyID) { LocalUser.LobbyID = InLobbyID; }
	void SetShadowLobbyID(const uint64 InShadowLobbyID) { LocalUser.ShadowLobbyID = InShadowLobbyID; }
	void SetEpicAccountId(const EOS_EpicAccountId EpicAccountId) { LocalUser.EpicAccountID = EpicAccountId; }
	void SetProductUserId(const EOS_ProductUserId ProductUserId) { LocalUser.ProductUserId = ProductUserId; }
	void SetContinuanceToken(const EOS_ContinuanceToken ContinuanceToken) { LocalUser.ContinuanceToken = ContinuanceToken; }
	void SetPlatformType(const EPlatformType PlatformType) { LocalUser.PlatformType = PlatformType; }


	
	FORCEINLINE bool IsAuthLoggedIn() const { return EOS_EpicAccountId_IsValid(LocalUser.EpicAccountID) == EOS_TRUE; }
	FORCEINLINE bool IsConnectLoggedIn() const { return EOS_ProductUserId_IsValid(LocalUser.ProductUserId) == EOS_TRUE; }

	FORCEINLINE bool IsInLobby() const { return LocalUser.LobbyID != nullptr && LocalUser.LobbyID[0] != '\0'; }
	FORCEINLINE bool IsInShadowLobby() const { return LocalUser.LobbyID != 0; }
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

	~FEosManager();
	
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
	TSharedPtr<FLocalUserState> LocalUserState;

public:
	FORCEINLINE void SetRichPresence(const char *Key, const char *Value) const {SteamManager->SetRichPresence(Key, Value);}
	FORCEINLINE EOS_HPlatform GetPlatformHandle() const { return PlatformHandle; }
	FORCEINLINE TSharedPtr<FLocalUserState> GetLocalUserState() const { return LocalUserState; }
	FORCEINLINE CSteamID GetSteamID() const { return SteamManager->GetSteamID(); }
};

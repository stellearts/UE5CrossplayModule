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

private:
	class FSteamManager& SteamManager;
	class FEosManager& EosManager;
	UPROPERTY() class USteamUserSubsystem* SteamUserSubsystem;

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

private:
	FLocalUserState UserState;

public:
	// UserState Getters
	FORCEINLINE EOS_ProductUserId GetProductUserID() const { return UserState.ProductUserID; }
	FORCEINLINE EOS_EpicAccountId GetEpicAccountID() const { return UserState.EpicAccountID; }
	FORCEINLINE EOS_EExternalAccountType GetPlatform() const { return UserState.Platform; }
	FORCEINLINE std::string GetPlatformID() const { return UserState.PlatformID; }
	FORCEINLINE std::string GetDisplayName() const { return UserState.DisplayName; }
	FORCEINLINE std::string GetAvatarURL() const { return UserState.AvatarURL; }
	FORCEINLINE std::string GetLobbyID() const { return UserState.LobbyID; }
	FORCEINLINE uint64 GetShadowLobbyID() const { return UserState.ShadowLobbyID; }
	FORCEINLINE EOS_ContinuanceToken GetContinuanceToken() const { return UserState.ContinuanceToken; }

	// UserState Setters
	void SetProductUserId(const EOS_ProductUserId ProductUserId) { UserState.ProductUserID = ProductUserId; }
	void SetEpicAccountId(const EOS_EpicAccountId EpicAccountId) { UserState.EpicAccountID = EpicAccountId; }
	void SetPlatform(const EOS_EExternalAccountType PlatformType) { UserState.Platform = PlatformType; }
	void SetPlatformID(const std::string PlatformID) { UserState.PlatformID = PlatformID; }
	void SetDisplayName(const std::string DisplayName) { UserState.DisplayName = DisplayName; }
	void SetAvatarURL(const std::string AvatarURL) { UserState.AvatarURL = AvatarURL; }
	void SetContinuanceToken(const EOS_ContinuanceToken InContinuanceToken) { UserState.ContinuanceToken = InContinuanceToken; }
	void SetLobbyID(const std::string& InLobbyID) { UserState.LobbyID = InLobbyID; }
	void SetShadowLobbyID(const uint64 InShadowLobbyID) { UserState.ShadowLobbyID = InShadowLobbyID; }

	// UserState helper functions to make the code more readable.
	FORCEINLINE bool IsAuthLoggedIn() const { return EOS_EpicAccountId_IsValid(UserState.EpicAccountID) == EOS_TRUE; }
	FORCEINLINE bool IsConnectLoggedIn() const { return EOS_ProductUserId_IsValid(UserState.ProductUserID) == EOS_TRUE; }
	FORCEINLINE bool IsInLobby() const { return !UserState.LobbyID.empty(); }
	FORCEINLINE bool IsInShadowLobby() const { return UserState.ShadowLobbyID != 0; }


	
	// ------------------------------- Local User Utilities -------------------------------
	
	void RequestSteamEncryptedAppTicket(const TFunction<void(std::string Ticket)> TicketReadyCallback);
	void RequestSteamSessionTicket(const TFunction<void(std::string Ticket)> TicketReadyCallback);

private:
	void OnSteamEncryptedAppTicketResponse(const TArray<uint8> Ticket);
	TFunction<void(std::string TicketString)> SteamEncryptedAppTicketCallback;
	void OnSteamSessionTicketResponse(const TArray<uint8> Ticket);
	TFunction<void(std::string TicketString)> SteamSessionTicketCallback;
};

// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "eos_lobby_types.h"
#include "EOS/EOSManager.h"
#include "LocalUserStateSubsystem.generated.h"



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
 * Subsystem for keeping track of the local user's state.
 */
UCLASS()
class ONLINEMULTIPLAYER_API ULocalUserStateSubsystem final : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	/**
	 * Struct that holds information about a local user.
	 */
	struct FLocalUser
	{
		std::string LobbyID; // EOS_LobbyId is a typedef for 'const char*' so we can use std::string here. Prevents it from being deallocated.
		uint64 ShadowLobbyID = 0;
		EOS_EpicAccountId EpicAccountID;
		EOS_ProductUserId ProductUserID;
		EOS_ContinuanceToken ContinuanceToken;
		EPlatformType PlatformType = EPlatformType::PlatformType_None;
	};
	FLocalUser LocalUser;
	
public:
	FORCEINLINE EOS_LobbyId GetLobbyID() const { return LocalUser.LobbyID.c_str(); }
	FORCEINLINE uint64 GetShadowLobbyID() const { return LocalUser.ShadowLobbyID; }
	FORCEINLINE EOS_EpicAccountId GetEpicAccountID() const { return LocalUser.EpicAccountID; }
	FORCEINLINE EOS_ProductUserId GetProductUserID() const { return LocalUser.ProductUserID; }
	FORCEINLINE EOS_ContinuanceToken GetContinuanceToken() const { return LocalUser.ContinuanceToken; }
	FORCEINLINE EPlatformType GetPlatformType() const { return LocalUser.PlatformType; }
	
	void SetLobbyID(const EOS_LobbyId InLobbyID) { LocalUser.LobbyID = InLobbyID; }
	void SetShadowLobbyID(const uint64 InShadowLobbyID) { LocalUser.ShadowLobbyID = InShadowLobbyID; }
	void SetEpicAccountId(const EOS_EpicAccountId EpicAccountId) { LocalUser.EpicAccountID = EpicAccountId; }
	void SetProductUserId(const EOS_ProductUserId ProductUserId) { LocalUser.ProductUserID = ProductUserId; }
	void SetContinuanceToken(const EOS_ContinuanceToken ContinuanceToken) { LocalUser.ContinuanceToken = ContinuanceToken; }
	void SetPlatformType(const EPlatformType PlatformType) { LocalUser.PlatformType = PlatformType; }
	
	// Helper functions to make the code more readable.
	FORCEINLINE bool IsAuthLoggedIn() const { return EOS_EpicAccountId_IsValid(LocalUser.EpicAccountID) == EOS_TRUE; }
	FORCEINLINE bool IsConnectLoggedIn() const { return EOS_ProductUserId_IsValid(LocalUser.ProductUserID) == EOS_TRUE; }
	FORCEINLINE bool IsInLobby() const { return !LocalUser.LobbyID.empty(); }
	FORCEINLINE bool IsInShadowLobby() const { return LocalUser.ShadowLobbyID != 0; }
};

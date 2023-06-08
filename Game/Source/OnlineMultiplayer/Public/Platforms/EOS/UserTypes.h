// Copyright © 2023 Melvin Brink

#pragma once

#include <string>
#include "eos_common.h"
#include "UserTypes.generated.h"




// TODO: UUser class for platform specific. UEosUser : UUser for online users. ULocalUser : UEosUser for some extra local functionality.

// These are the external account types that EOS supports
struct FExternalAccount {
	EOS_ProductUserId ProductUserID;
	std::string DisplayName;
	std::string AccountID;
	EOS_EExternalAccountType AccountType;
	int64_t LastLoginTime;
};
typedef TMap<EOS_EExternalAccountType, FExternalAccount> FExternalAccountsMap;



/**
 * The platform user contains the data that any platform can supply, such as getting the username or avatar of a Steam or PSN user, and should be used for the friends on the users platform.
 */
UCLASS(BlueprintType)
class UPlatformUser : public UObject
{
	GENERATED_BODY()

	struct FPlatformUserState
	{
		std::string DisplayName;
		std::string PlatformUserID;
		std::string AvatarURL;
	};

protected:
	FPlatformUserState PlatformUserState;

public:
	void Initialize(const std::string& InPlatformUserID, const std::string& InDisplayName)
	{
		PlatformUserState.PlatformUserID = InPlatformUserID;
		PlatformUserState.DisplayName = InDisplayName;
	}
	
	// Getters
	FORCEINLINE std::string GetPlatformID() const { return PlatformUserState.PlatformUserID; }
	FORCEINLINE std::string GetDisplayName() const { return PlatformUserState.DisplayName; }
	FORCEINLINE std::string GetAvatarURL() const { return PlatformUserState.AvatarURL; }

	// Setters
	FORCEINLINE void SetPlatformID(const std::string PlatformUserID) { PlatformUserState.PlatformUserID = PlatformUserID; }
	FORCEINLINE void SetDisplayName(const std::string DisplayName) { PlatformUserState.DisplayName = DisplayName; }
	FORCEINLINE void SetAvatarURL(const std::string AvatarURL) { PlatformUserState.AvatarURL = AvatarURL; }
};
typedef TStrongObjectPtr<UPlatformUser> FPlatformUserPtr;

// For storing platform users.
typedef TMap<FString, FPlatformUserPtr> FPlatformUserMap; // FString because std::string is not a natively supported key type for UE or something.



/**
 * Represents an online user which has the extra properties we need to do matchmaking.
 *
 * Users joining a lobby/session will join with their EOS-product-user-ID and we can use that to fetch some data and create this user.
 */
UCLASS(BlueprintType)
class UEosUser : public UPlatformUser
{
	GENERATED_BODY()
	
	struct FUserState
	{
		EOS_ProductUserId ProductUserID;
		EOS_EpicAccountId EpicAccountID;
		FExternalAccountsMap ExternalAccounts;
		EOS_EExternalAccountType Platform;
	};

protected:
	FUserState UserState;
	
public:
	void Initialize(
			const EOS_ProductUserId& InProductUserID,
			const EOS_EpicAccountId& InEpicAccountID,
			const FExternalAccountsMap& InExternalAccounts,
			const EOS_EExternalAccountType& InPlatform,
			const std::string& InPlatformUserID,
			const std::string& InDisplayName)
	{
		UserState.ProductUserID = InProductUserID;
		UserState.EpicAccountID = InEpicAccountID;
		UserState.ExternalAccounts = InExternalAccounts;
		UserState.Platform = InPlatform;
		PlatformUserState.PlatformUserID = InPlatformUserID;
		PlatformUserState.DisplayName = InDisplayName;
	}
	
	// Getters
	FORCEINLINE EOS_ProductUserId GetProductUserID() const { return UserState.ProductUserID; }
	FORCEINLINE EOS_EpicAccountId GetEpicAccountID() const { return UserState.EpicAccountID; }
	FORCEINLINE EOS_EExternalAccountType GetPlatform() const { return UserState.Platform; }

	// Setters
	FORCEINLINE void SetProductUserID(const EOS_ProductUserId ProductUserId) { UserState.ProductUserID = ProductUserId; }
	FORCEINLINE void SetEpicAccountID(const EOS_EpicAccountId EpicAccountId) { UserState.EpicAccountID = EpicAccountId; }
	FORCEINLINE void SetPlatform(const EOS_EExternalAccountType PlatformType) { UserState.Platform = PlatformType; }
};
typedef TStrongObjectPtr<UEosUser> FEosUserPtr;

// For storing online users.
typedef TMap<EOS_ProductUserId, FEosUserPtr> FEosUserMap;
// TODO: To remove an item from the list, first call the '.Reset();' on the strongobjectptr, and then remove it from the list. Otherwise memory leak.



/**
 * The local user is an EOS user with some extra data and helper methods.
 */
UCLASS(BlueprintType)
class ULocalUser final : public UEosUser
{
	GENERATED_BODY()
	
	struct FLocalUserState
	{
		std::string LobbyID;
		uint64 ShadowLobbyID = 0;
		EOS_ContinuanceToken ContinuanceToken;
	};
	FLocalUserState LocalUserState;
	
public:
	// Getters
	FORCEINLINE std::string GetLobbyID() const { return LocalUserState.LobbyID; }
	FORCEINLINE uint64 GetShadowLobbyID() const { return LocalUserState.ShadowLobbyID; }
	FORCEINLINE EOS_ContinuanceToken GetContinuanceToken() const { return LocalUserState.ContinuanceToken; }

	// Setters
	FORCEINLINE void SetContinuanceToken(const EOS_ContinuanceToken& InContinuanceToken) { LocalUserState.ContinuanceToken = InContinuanceToken; }
	FORCEINLINE void SetLobbyID(const std::string& InLobbyID) { LocalUserState.LobbyID = InLobbyID; }
	FORCEINLINE void SetShadowLobbyID(const uint64 InShadowLobbyID) { LocalUserState.ShadowLobbyID = InShadowLobbyID; }

	// Helper functions to make the code more readable
	FORCEINLINE bool IsAuthLoggedIn() const { return EOS_EpicAccountId_IsValid(UserState.EpicAccountID) == EOS_TRUE; }
	FORCEINLINE bool IsConnectLoggedIn() const { return EOS_ProductUserId_IsValid(UserState.ProductUserID) == EOS_TRUE; }
	FORCEINLINE bool IsInLobby() const { return !LocalUserState.LobbyID.empty(); }
	FORCEINLINE bool IsInShadowLobby() const { return LocalUserState.ShadowLobbyID != 0; }
};
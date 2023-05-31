// Copyright © 2023 Melvin Brink

#pragma once

#include <string>
#include "eos_common.h"
#include "UserTypes.generated.h"



// These are the external account types that EOS supports->
struct FExternalAccount {
	EOS_ProductUserId ProductUserID;
	std::string DisplayName;
	std::string AccountID;
	EOS_EExternalAccountType AccountType;
	int64_t LastLoginTime;
};
typedef TMap<EOS_EExternalAccountType, FExternalAccount> FExternalAccountsMap;


UCLASS(BlueprintType)
class UUser : public UObject
{
	GENERATED_BODY()
	
	struct FUserState
	{
		EOS_ProductUserId ProductUserID;
		EOS_EpicAccountId EpicAccountID;
		FExternalAccountsMap ExternalAccounts;
		EOS_EExternalAccountType Platform;
		std::string PlatformID;
		std::string DisplayName;
		std::string AvatarURL;
	};

protected:
	FUserState* UserState;
	
public:
	void Initialize(
			const EOS_ProductUserId& InProductUserID,
			const EOS_EpicAccountId& InEpicAccountID,
			const FExternalAccountsMap& InExternalAccounts,
			const EOS_EExternalAccountType& InPlatform,
			const std::string& InPlatformID,
			const std::string& InDisplayName)
	{
		UserState->ProductUserID = InProductUserID;
		UserState->EpicAccountID = InEpicAccountID;
		UserState->ExternalAccounts = InExternalAccounts;
		UserState->Platform = InPlatform;
		UserState->PlatformID = InPlatformID;
		UserState->DisplayName = InDisplayName;
	}
	
	
	// Getters
	FORCEINLINE EOS_ProductUserId GetProductUserID() const { return UserState->ProductUserID; }
	FORCEINLINE EOS_EpicAccountId GetEpicAccountID() const { return UserState->EpicAccountID; }
	FORCEINLINE EOS_EExternalAccountType GetPlatform() const { return UserState->Platform; }
	FORCEINLINE std::string GetPlatformID() const { return UserState->PlatformID; }
	FORCEINLINE std::string GetDisplayName() const { return UserState->DisplayName; }
	FORCEINLINE std::string GetAvatarURL() const { return UserState->AvatarURL; }

	// Setters
	FORCEINLINE void SetProductUserID(const EOS_ProductUserId ProductUserId) { UserState->ProductUserID = ProductUserId; }
	FORCEINLINE void SetEpicAccountID(const EOS_EpicAccountId EpicAccountId) { UserState->EpicAccountID = EpicAccountId; }
	FORCEINLINE void SetPlatform(const EOS_EExternalAccountType PlatformType) { UserState->Platform = PlatformType; }
	FORCEINLINE void SetPlatformID(const std::string PlatformID) { UserState->PlatformID = PlatformID; }
	FORCEINLINE void SetDisplayName(const std::string DisplayName) { UserState->DisplayName = DisplayName; }
	FORCEINLINE void SetAvatarURL(const std::string AvatarURL) { UserState->AvatarURL = AvatarURL; }
};
// Use this map to store a list of users that can be found using the ProductUserID->
typedef TMap<EOS_ProductUserId, TStrongObjectPtr<UUser>> FUsersMap;
// TODO: FUsersMap to EOSUsersMap and maps for each platform? FSteamUsersMap, psn, etc.



// Local User class that handles the information for a local user->
UCLASS(BlueprintType)
class ULocalUser final : public UUser
{
	GENERATED_BODY()
	
	struct FLocalUserState
	{
		std::string LobbyID;
		uint64 ShadowLobbyID = 0;
		EOS_ContinuanceToken ContinuanceToken;
	};
	FLocalUserState* LocalUserState;
	
public:
	ULocalUser() = default;
	
	// Getters
	FORCEINLINE std::string GetLobbyID() const { return LocalUserState->LobbyID; }
	FORCEINLINE uint64 GetShadowLobbyID() const { return LocalUserState->ShadowLobbyID; }
	FORCEINLINE EOS_ContinuanceToken GetContinuanceToken() const { return LocalUserState->ContinuanceToken; }

	// Setters
	FORCEINLINE void SetContinuanceToken(const EOS_ContinuanceToken& InContinuanceToken) { LocalUserState->ContinuanceToken = InContinuanceToken; }
	FORCEINLINE void SetLobbyID(const std::string& InLobbyID) { LocalUserState->LobbyID = InLobbyID; }
	FORCEINLINE void SetShadowLobbyID(const uint64 InShadowLobbyID) { LocalUserState->ShadowLobbyID = InShadowLobbyID; }

	// Helper functions to make the code more readable
	FORCEINLINE bool IsAuthLoggedIn() const { return EOS_EpicAccountId_IsValid(UserState->EpicAccountID) == EOS_TRUE; }
	FORCEINLINE bool IsConnectLoggedIn() const { return EOS_ProductUserId_IsValid(UserState->ProductUserID) == EOS_TRUE; }
	FORCEINLINE bool IsInLobby() const { return !LocalUserState->LobbyID.empty(); }
	FORCEINLINE bool IsInShadowLobby() const { return LocalUserState->ShadowLobbyID != 0; }
};
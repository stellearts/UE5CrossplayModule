#pragma once

#include <string>
#include "eos_common.h"


// These are the external account types that EOS supports.
struct FExternalAccount {
	EOS_ProductUserId ProductUserID;
	std::string DisplayName;
	std::string AccountID;
	EOS_EExternalAccountType AccountType;
	int64_t LastLoginTime;
};
typedef TMap<EOS_EExternalAccountType, FExternalAccount> FExternalAccountsMap;


// This is the base user struct that contains all the information we need to know about a user. Can be used for online users.
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
// Use this map to store a list of users that can be found using the ProductUserID.
typedef TMap<EOS_ProductUserId, FUserState> FUserStateMap;


// Has the extra information that is needed for the local user.
struct FLocalUserState : FUserState
{
	std::string LobbyID;
	uint64 ShadowLobbyID = 0;
	EOS_ContinuanceToken ContinuanceToken;
};
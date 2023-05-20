// Copyright © 2023 Melvin Brink

#pragma once

#include "eos_common.h"
#include "OnlineMultiplayer_CommonTypes.generated.h"



struct FExternalAccount {
	EOS_ProductUserId ProductUserID;
	std::string DisplayName;
	std::string AccountID;
	EOS_EExternalAccountType AccountType;
	int64_t LastLoginTime;
};
typedef TMap<EOS_EExternalAccountType, FExternalAccount> FExternalAccountsMap;


USTRUCT(BlueprintType)
struct FOnlineUser
{
	GENERATED_BODY()
	
	EOS_ProductUserId ProductUserID;
	EOS_EpicAccountId EpicAccountID;
	FExternalAccountsMap ExternalAccounts;
	EOS_EExternalAccountType Platform;
	std::string PlatformID;
	std::string DisplayName;
	std::string AvatarURL;
	bool bInLobby;
	bool bInSession;
};
typedef TMap<EOS_ProductUserId, FOnlineUser> FOnlineUserMap;

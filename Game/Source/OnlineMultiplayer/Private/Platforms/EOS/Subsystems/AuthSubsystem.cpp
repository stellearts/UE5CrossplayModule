// Copyright © 2023 Melvin Brink

#include "Platforms/EOS/Subsystems/AuthSubsystem.h"
#include "Platforms/EOS/EOSManager.h"
#include "eos_auth.h"
#include "UserStateSubsystem.h"




void UAuthSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	SteamManager = &FSteamManager::Get();

	// Make sure the local user state subsystem is initialized.
	LocalUserState = Collection.InitializeDependency<UUserStateSubsystem>();

	EosManager = &FEosManager::Get();
	const EOS_HPlatform PlatformHandle = EosManager->GetPlatformHandle();
	if(!PlatformHandle) return;
	AuthHandle = EOS_Platform_GetAuthInterface(PlatformHandle);
	ConnectHandle = EOS_Platform_GetConnectInterface(PlatformHandle);
	
	LoginConnect();
}


// --------------------------------------------


void UAuthSubsystem::LoginAuth()
{
	if(!AuthHandle) return;
	EosManager->RequestSteamSessionTicket([this](std::string TicketString)
	{
		EOS_Auth_Credentials Credentials;
		Credentials.ApiVersion = EOS_AUTH_CREDENTIALS_API_LATEST;
		Credentials.Type = EOS_ELoginCredentialType::EOS_LCT_ExternalAuth;
		Credentials.ExternalType = EOS_EExternalCredentialType::EOS_ECT_STEAM_SESSION_TICKET;
		Credentials.Token = TicketString.c_str();
	
		EOS_Auth_LoginOptions Options;
		Options.ApiVersion = EOS_AUTH_LOGIN_API_LATEST;
		Options.Credentials = &Credentials;
		Options.ScopeFlags = EOS_EAuthScopeFlags::EOS_AS_Presence;
	
		EOS_Auth_Login(AuthHandle, &Options, this, OnLoginAuthComplete);
	});
}

void UAuthSubsystem::OnLoginAuthComplete(const EOS_Auth_LoginCallbackInfo* Data)
{
	UAuthSubsystem* AuthSubsystem = static_cast<UAuthSubsystem*>(Data->ClientData);
	if(!AuthSubsystem) return;
	UUserStateSubsystem* LocalUserState = AuthSubsystem->LocalUserState;
	if(!LocalUserState) return;
	
	if(Data->ResultCode == EOS_EResult::EOS_Success)
	{
		// Login was successful. We can now use the Auth interface.
		LocalUserState->SetEpicAccountId(Data->LocalUserId);
	}
	else if(Data->ResultCode == EOS_EResult::EOS_Auth_ExternalAuthNotLinked ||
			Data->ResultCode == EOS_EResult::EOS_InvalidUser)
	{
		// Open the login overlay. The user can now login with their Epic account, or create a new one.
		LocalUserState->SetContinuanceToken(Data->ContinuanceToken);
		AuthSubsystem->LinkUserAuth();
	}
	else
	{
		UE_LOG(LogAuthSubsystem, Error, TEXT("LoginAuth failed with error code %d"), Data->ResultCode);
	}
}

void UAuthSubsystem::LogoutAuth()
{
}

void UAuthSubsystem::OnLogoutAuthComplete()
{
}


// --------------------------------------------


void UAuthSubsystem::LoginConnect()
{
	if(!ConnectHandle) return;
	EosManager->RequestSteamSessionTicket([this](std::string TicketString)
	{
		EOS_Connect_Credentials Credentials;
		Credentials.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
		Credentials.Type = EOS_EExternalCredentialType::EOS_ECT_STEAM_SESSION_TICKET;
		// Credentials.Type = EOS_EExternalCredentialType::EOS_ECT_STEAM_APP_TICKET;
		Credentials.Token = TicketString.c_str();

		EOS_Connect_LoginOptions Options;
		Options.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;
		Options.Credentials = &Credentials;
		Options.UserLoginInfo = nullptr;

		EOS_Connect_Login(ConnectHandle, &Options, this, OnLoginConnectComplete);
	});
}

void UAuthSubsystem::LogoutConnect()
{
	
}

void UAuthSubsystem::OnLoginConnectComplete(const EOS_Connect_LoginCallbackInfo* Data)
{
	UAuthSubsystem* AuthSubsystem = static_cast<UAuthSubsystem*>(Data->ClientData);
	if(!AuthSubsystem) return;
	UUserStateSubsystem* LocalUserState = AuthSubsystem->LocalUserState;
	if(!LocalUserState) return;
	
	if (Data->ResultCode == EOS_EResult::EOS_Success)
	{
		LocalUserState->SetProductUserId(Data->LocalUserId);
	}
	else if(Data->ResultCode == EOS_EResult::EOS_InvalidUser)
	{
		// Create a new account. But maybe check if the user wants to do this.
		LocalUserState->SetContinuanceToken(Data->ContinuanceToken);
		AuthSubsystem->CreateNewUserConnect();
	}
	else
	{
		UE_LOG(LogAuthSubsystem, Error, TEXT("LoginConnect failed with error code %d"), Data->ResultCode);
	}
}

void UAuthSubsystem::OnLogoutConnectComplete()
{
}


// --------------------------------------------


void UAuthSubsystem::LinkUserAuth()
{
	if(!LocalUserState) return;
	
	EOS_Auth_LinkAccountOptions Options;
	Options.ApiVersion = EOS_AUTH_LINKACCOUNT_API_LATEST;
	Options.LinkAccountFlags = EOS_ELinkAccountFlags::EOS_LA_NoFlags;
	Options.LocalUserId = nullptr;
	Options.ContinuanceToken = LocalUserState->GetContinuanceToken();
	
	EOS_Auth_LinkAccount(AuthHandle, &Options, this, [](const EOS_Auth_LinkAccountCallbackInfo* Data)
	{
		UAuthSubsystem* AuthSubsystem = static_cast<UAuthSubsystem*>(Data->ClientData);
		if(!AuthSubsystem) return;
		
		if(Data->ResultCode == EOS_EResult::EOS_Success)
		{
			UE_LOG(LogAuthSubsystem, Display, TEXT("LinkAccount success"));
			AuthSubsystem->LoginAuth();
		}
		else if(Data->ResultCode == EOS_EResult::EOS_Canceled)
		{
			// User canceled the login process.
		}
		else
		{
			UE_LOG(LogAuthSubsystem, Error, TEXT("LinkAccount failed with error code %d"), Data->ResultCode);
		}
	});
}

/**
 * Creates a new user account.
 * 
 * Associates the current platform with the Product User ID of this new account.
 */
void UAuthSubsystem::CreateNewUserConnect()
{
	if(!LocalUserState) return;
	
	EOS_Connect_CreateUserOptions CreateUserOptions;
	CreateUserOptions.ApiVersion = EOS_CONNECT_CREATEUSER_API_LATEST;
	CreateUserOptions.ContinuanceToken = LocalUserState->GetContinuanceToken();
	EOS_Connect_CreateUser(ConnectHandle, &CreateUserOptions, this, [](const EOS_Connect_CreateUserCallbackInfo* Data)
	{
		UAuthSubsystem* AuthSubsystem = static_cast<UAuthSubsystem*>(Data->ClientData);
		if(!AuthSubsystem) return;
		
		if(Data->ResultCode == EOS_EResult::EOS_Success)
		{
			UE_LOG(LogAuthSubsystem, Display, TEXT("CreateUser success"));
			AuthSubsystem->LoginConnect();
		}
		else
		{
			UE_LOG(LogAuthSubsystem, Error, TEXT("CreateUser failed with error code %d"), Data->ResultCode);
		}
	});
}

void UAuthSubsystem::CheckAccounts()
{
	if(!LocalUserState) return;
	
	EOS_Connect_QueryProductUserIdMappingsOptions QueryMappingsOptions = {};
	QueryMappingsOptions.ApiVersion = EOS_CONNECT_QUERYPRODUCTUSERIDMAPPINGS_API_LATEST;
	QueryMappingsOptions.LocalUserId = LocalUserState->GetProductUserID();
	EOS_ProductUserId ProductUserIds[] = {QueryMappingsOptions.LocalUserId};
	QueryMappingsOptions.ProductUserIds = ProductUserIds;
	QueryMappingsOptions.ProductUserIdCount = 1;

	EOS_Connect_QueryProductUserIdMappings(ConnectHandle, &QueryMappingsOptions, this,[](const EOS_Connect_QueryProductUserIdMappingsCallbackInfo* Data)
	{
		if(Data->ResultCode != EOS_EResult::EOS_Success)
		{
			UE_LOG(LogAuthSubsystem, Error, TEXT("QueryProductUserIdMappings failed with error code %d"), Data->ResultCode);
			return;
		}
		
		const UAuthSubsystem* AuthSubsystem = static_cast<UAuthSubsystem*>(Data->ClientData);
		EOS_Connect_CopyProductUserExternalAccountByAccountTypeOptions Options = {};
		Options.ApiVersion = EOS_CONNECT_COPYPRODUCTUSEREXTERNALACCOUNTBYACCOUNTTYPE_API_LATEST;
		Options.TargetUserId = Data->LocalUserId;
		Options.AccountIdType = EOS_EExternalAccountType::EOS_EAT_STEAM;

		EOS_Connect_ExternalAccountInfo* ExternalAccountInfo;
		EOS_EResult Result = EOS_Connect_CopyProductUserExternalAccountByAccountType(AuthSubsystem->ConnectHandle, &Options, &ExternalAccountInfo);
		if (Result != EOS_EResult::EOS_Success)
		{
			UE_LOG(LogAuthSubsystem, Error, TEXT("Steam account is not linked or an error occurred, error code %d"), Result);
			return;
		}
		
		UE_LOG(LogAuthSubsystem, Log, TEXT("Steam account is linked"));
		
		// Release the memory for ExternalAccountInfo when done.
		EOS_Connect_ExternalAccountInfo_Release(ExternalAccountInfo);
	});
}


// --------------------------------------------

struct FGetUserInfoCallbackData
{
	UAuthSubsystem* AuthSubsystem;
	UUserStateSubsystem* UserStateSubsystem;
	TArray<EOS_ProductUserId> UserIDs;
};

/**
 * Get the necessary info for the given Product User IDs.
 * This also returns the external account info for the given Product User IDs and binds it to each user.
 *
 * @param UserIDs Product-User-IDs array to get the external accounts info for.
 * @param Callback The callback to call upon completion, returning FOnlineUserMap.
 */
void UAuthSubsystem::GetUserInfo(TArray<EOS_ProductUserId>& UserIDs, const TFunction<void(FOnlineUserMap)> Callback)
{
	// Data we need in the OnGetProductUserExternalAccountInfoComplete
	const TUniquePtr<FGetUserInfoCallbackData> CallbackData = MakeUnique<FGetUserInfoCallbackData>();
	CallbackData->AuthSubsystem = this;
	CallbackData->UserStateSubsystem = LocalUserState;
	CallbackData->UserIDs = UserIDs;
	
	EOS_Connect_QueryProductUserIdMappingsOptions Options;
	Options.ApiVersion = EOS_CONNECT_QUERYPRODUCTUSERIDMAPPINGS_API_LATEST;
	Options.LocalUserId = LocalUserState->GetProductUserID();
	Options.ProductUserIds = UserIDs.GetData();
	Options.ProductUserIdCount = UserIDs.Num();

	GetUserInfoCallback = Callback;
	EOS_Connect_QueryProductUserIdMappings(ConnectHandle, &Options, CallbackData.Get(), &UAuthSubsystem::OnGetUserInfoComplete);
}

void UAuthSubsystem::OnGetUserInfoComplete(const EOS_Connect_QueryProductUserIdMappingsCallbackInfo* Data)
{
	const FGetUserInfoCallbackData* CallbackData = static_cast<FGetUserInfoCallbackData*>(Data->ClientData);
	UAuthSubsystem* AuthSubsystem = CallbackData->AuthSubsystem;
	
	if (Data->ResultCode != EOS_EResult::EOS_Success)
	{
		UE_LOG(LogAuthSubsystem, Error, TEXT("EOS_Connect_QueryProductUserIdMappings failed with error code: [%d]"), Data->ResultCode);
		AuthSubsystem->GetUserInfoCallback(FOnlineUserMap());
		return;
	}
	
	const TArray<EOS_ProductUserId>& UserIDs = CallbackData->UserIDs;
	FOnlineUserMap OnlineUsers = {}; // To pass to the callback.
	
	// Iterate over all the users we requested info for.
	for(int32_t UserIndex = 0; UserIndex < UserIDs.Num(); UserIndex++)
	{
		const EOS_ProductUserId TargetUserID = UserIDs[UserIndex];
		EOS_Connect_GetProductUserExternalAccountCountOptions ExternalAccountCountOptions;
		ExternalAccountCountOptions.ApiVersion = EOS_CONNECT_GETPRODUCTUSEREXTERNALACCOUNTCOUNT_API_LATEST;
		ExternalAccountCountOptions.TargetUserId = TargetUserID;

		FExternalAccountsMap ExternalAccounts;
		const int32_t AccountCount = EOS_Connect_GetProductUserExternalAccountCount(AuthSubsystem->ConnectHandle, &ExternalAccountCountOptions);

		// Iterate over all the accounts for this user and add them to the ExternalAccounts map.
		for (int32_t AccountIndex = 0; AccountIndex < AccountCount; ++AccountIndex)
		{
			EOS_Connect_CopyProductUserExternalAccountByIndexOptions CopyOptions;
			CopyOptions.ApiVersion = EOS_CONNECT_COPYPRODUCTUSEREXTERNALACCOUNTBYINDEX_API_LATEST;
			CopyOptions.TargetUserId = TargetUserID;
			CopyOptions.ExternalAccountInfoIndex = AccountIndex;

			EOS_Connect_ExternalAccountInfo* EOS_AccountInfo;
			const EOS_EResult Result = EOS_Connect_CopyProductUserExternalAccountByIndex(AuthSubsystem->ConnectHandle, &CopyOptions, &EOS_AccountInfo);
			
			if (Result == EOS_EResult::EOS_Success)
			{
				FExternalAccount AccountInfo;
				AccountInfo.ProductUserID = EOS_AccountInfo->ProductUserId;
				AccountInfo.DisplayName = EOS_AccountInfo->DisplayName ? EOS_AccountInfo->DisplayName : "";
				AccountInfo.AccountID = EOS_AccountInfo->AccountId ? EOS_AccountInfo->AccountId : "";
				AccountInfo.AccountType = EOS_AccountInfo->AccountIdType;
				AccountInfo.LastLoginTime = EOS_AccountInfo->LastLoginTime;

				ExternalAccounts.Add(EOS_AccountInfo->AccountIdType, AccountInfo);
			}
			else
			{
				UE_LOG(LogAuthSubsystem, Error, TEXT("Failed to get external account info. Error code: %hs"), EOS_EResult_ToString(Result)); // Handle error.
			}

			EOS_Connect_ExternalAccountInfo_Release(EOS_AccountInfo);
		}

		// TODO: Try find a better way to determine logged in platform. This is not the best way, since a user can have multiple accounts and open the game on multiple devices. Which is a rare use case, but there should be a better way.
		// Get the platform the user is on.
		EOS_EExternalAccountType MostRecentPlatform = ExternalAccounts.CreateConstIterator().Value().AccountType;
		int64_t MostRecentLoginTime = 0;
		for (const auto& Account : ExternalAccounts)
		{
			if (Account.Value.LastLoginTime > MostRecentLoginTime)
			{
				MostRecentLoginTime = Account.Value.LastLoginTime;
				MostRecentPlatform = Account.Value.AccountType;
			}
		}

		UE_LOG(LogTemp, Warning, TEXT("Most recent platform: %d"), MostRecentPlatform);

		// Make the OnlineUser struct and add it to the OnlineUsers map.
		FOnlineUser OnlineUser;
		OnlineUser.ProductUserID = TargetUserID;
		OnlineUser.ExternalAccounts = ExternalAccounts;
		OnlineUser.Platform = MostRecentPlatform;
		OnlineUser.PlatformID = ExternalAccounts.Contains(MostRecentPlatform) ? ExternalAccounts[MostRecentPlatform].AccountID : "";
		OnlineUser.DisplayName = ExternalAccounts.Contains(MostRecentPlatform) ? ExternalAccounts[MostRecentPlatform].DisplayName : "Unknown";
		OnlineUsers.Add(TargetUserID, OnlineUser);
		
		// Load avatar from the user's platform.
		if(OnlineUser.Platform == EOS_EExternalAccountType::EOS_EAT_STEAM)
		{
			AuthSubsystem->SteamManager->GetUserAvatar(OnlineUser.PlatformID);
		}
		// TODO: Other platforms eventually.
	}
	
	AuthSubsystem->GetUserInfoCallback(OnlineUsers);
	AuthSubsystem->GetUserInfoCallback = TFunction<void(FOnlineUserMap)>(); // Clear the callback.
}
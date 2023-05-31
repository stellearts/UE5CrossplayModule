// Copyright © 2023 Melvin Brink

#include "Platforms/EOS/Subsystems/ConnectSubsystem.h"
#include "Platforms/EOS/Subsystems/LocalUserSubsystem.h"
#include "Platforms/EOS/EOSManager.h"
#include "eos_connect.h"
#include "UserStateSubsystem.h"



void UConnectSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	LocalUserSubsystem = Collection.InitializeDependency<ULocalUserSubsystem>();
	OnlineUserSubsystem = Collection.InitializeDependency<UOnlineUserSubsystem>();

	LocalUser = LocalUserSubsystem->GetLocalUser();

	EosManager = &FEosManager::Get();
	const EOS_HPlatform PlatformHandle = EosManager->GetPlatformHandle();
	if(!PlatformHandle) return;
	ConnectHandle = EOS_Platform_GetConnectInterface(PlatformHandle);
	
	Login();
}


// --------------------------------------------


void UConnectSubsystem::Login()
{
	// TODO: Also check if user is already logged in. Would prevent api call.
	if(!ConnectHandle) return;

	// TODO: Change to more generic name like GetLoginToken or something which will then call the correct function, depending on the platform. For steam: GetSteamSessionTicket.
	LocalUserSubsystem->RequestSteamSessionTicket([this](std::string TicketString)
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

		EOS_Connect_Login(ConnectHandle, &Options, this, OnLoginComplete);
	});
}

void UConnectSubsystem::Logout()
{
	
}

void UConnectSubsystem::OnLoginComplete(const EOS_Connect_LoginCallbackInfo* Data)
{
	UConnectSubsystem* ConnectSubsystem = static_cast<UConnectSubsystem*>(Data->ClientData);
	ULocalUser* LocalUser = ConnectSubsystem->LocalUserSubsystem->GetLocalUser();
	
	if (Data->ResultCode == EOS_EResult::EOS_Success)
	{
		LocalUser->SetProductUserID(Data->LocalUserId);
	}
	else if(Data->ResultCode == EOS_EResult::EOS_InvalidUser)
	{
		// Create a new account. But maybe check if the user wants to do this.
		LocalUser->SetContinuanceToken(Data->ContinuanceToken);
		ConnectSubsystem->CreateNewUser();
	}
	else
	{
		UE_LOG(LogConnectSubsystem, Error, TEXT("LoginConnect failed with error code %d"), Data->ResultCode);
	}
}

void UConnectSubsystem::OnLogoutComplete()
{
}


// --------------------------------------------


/**
 * Creates a new user account.
 * 
 * Associates the current platform with the Product User ID of this new account.
 */
void UConnectSubsystem::CreateNewUser()
{
	EOS_Connect_CreateUserOptions CreateUserOptions;
	CreateUserOptions.ApiVersion = EOS_CONNECT_CREATEUSER_API_LATEST;
	CreateUserOptions.ContinuanceToken = LocalUser->GetContinuanceToken();
	EOS_Connect_CreateUser(ConnectHandle, &CreateUserOptions, this, [](const EOS_Connect_CreateUserCallbackInfo* Data)
	{
		UConnectSubsystem* ConnectSubsystem = static_cast<UConnectSubsystem*>(Data->ClientData);
		if(!ConnectSubsystem) return;
		
		if(Data->ResultCode == EOS_EResult::EOS_Success)
		{
			UE_LOG(LogConnectSubsystem, Display, TEXT("CreateUser success"));
			ConnectSubsystem->Login();
		}
		else
		{
			UE_LOG(LogConnectSubsystem, Error, TEXT("CreateUser failed with error code %d"), Data->ResultCode);
		}
	});
}

void UConnectSubsystem::CheckAccounts()
{
	EOS_Connect_QueryProductUserIdMappingsOptions QueryMappingsOptions = {};
	QueryMappingsOptions.ApiVersion = EOS_CONNECT_QUERYPRODUCTUSERIDMAPPINGS_API_LATEST;
	QueryMappingsOptions.LocalUserId = LocalUser->GetProductUserID();
	EOS_ProductUserId ProductUserIds[] = {QueryMappingsOptions.LocalUserId};
	QueryMappingsOptions.ProductUserIds = ProductUserIds;
	QueryMappingsOptions.ProductUserIdCount = 1;

	EOS_Connect_QueryProductUserIdMappings(ConnectHandle, &QueryMappingsOptions, this,[](const EOS_Connect_QueryProductUserIdMappingsCallbackInfo* Data)
	{
		if(Data->ResultCode != EOS_EResult::EOS_Success)
		{
			UE_LOG(LogConnectSubsystem, Error, TEXT("QueryProductUserIdMappings failed with error code %d"), Data->ResultCode);
			return;
		}
		
		const UConnectSubsystem* ConnectSubsystem = static_cast<UConnectSubsystem*>(Data->ClientData);
		EOS_Connect_CopyProductUserExternalAccountByAccountTypeOptions Options;
		Options.ApiVersion = EOS_CONNECT_COPYPRODUCTUSEREXTERNALACCOUNTBYACCOUNTTYPE_API_LATEST;
		Options.TargetUserId = Data->LocalUserId;
		Options.AccountIdType = EOS_EExternalAccountType::EOS_EAT_STEAM;

		EOS_Connect_ExternalAccountInfo* ExternalAccountInfo;
		const EOS_EResult Result = EOS_Connect_CopyProductUserExternalAccountByAccountType(ConnectSubsystem->ConnectHandle, &Options, &ExternalAccountInfo);
		if (Result != EOS_EResult::EOS_Success)
		{
			UE_LOG(LogConnectSubsystem, Error, TEXT("Steam account is not linked or an error occurred, error code %d"), Result);
			return;
		}
		
		UE_LOG(LogConnectSubsystem, Log, TEXT("Steam account is linked"));
		
		// Release the memory for ExternalAccountInfo when done.
		EOS_Connect_ExternalAccountInfo_Release(ExternalAccountInfo);
	});
}


// --------------------------------------------

struct FGetUserInfoCallbackData
{
	UConnectSubsystem* ConnectSubsystem;
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
void UConnectSubsystem::GetUserInfo(TArray<EOS_ProductUserId>& UserIDs, const TFunction<void(FUsersMap)> Callback)
{
	// Data we need in the OnGetProductUserExternalAccountInfoComplete
	const TUniquePtr<FGetUserInfoCallbackData> CallbackData = MakeUnique<FGetUserInfoCallbackData>();
	CallbackData->ConnectSubsystem = this;
	CallbackData->UserIDs = UserIDs;
	
	EOS_Connect_QueryProductUserIdMappingsOptions Options;
	Options.ApiVersion = EOS_CONNECT_QUERYPRODUCTUSERIDMAPPINGS_API_LATEST;
	Options.LocalUserId = LocalUser->GetProductUserID();
	Options.ProductUserIds = UserIDs.GetData();
	Options.ProductUserIdCount = UserIDs.Num();

	GetUserInfoCallback = Callback;
	EOS_Connect_QueryProductUserIdMappings(ConnectHandle, &Options, CallbackData.Get(), &UConnectSubsystem::OnGetUserInfoComplete);
}

void UConnectSubsystem::OnGetUserInfoComplete(const EOS_Connect_QueryProductUserIdMappingsCallbackInfo* Data)
{
	const FGetUserInfoCallbackData* CallbackData = static_cast<FGetUserInfoCallbackData*>(Data->ClientData);
	UConnectSubsystem* ConnectSubsystem = CallbackData->ConnectSubsystem;
	
	if (Data->ResultCode != EOS_EResult::EOS_Success)
	{
		UE_LOG(LogConnectSubsystem, Error, TEXT("EOS_Connect_QueryProductUserIdMappings failed with error code: [%d]"), Data->ResultCode);
		ConnectSubsystem->GetUserInfoCallback(FUsersMap());
		return;
	}
	
	const TArray<EOS_ProductUserId>& UserIDs = CallbackData->UserIDs;
	FUsersMap OnlineUsers = {}; // To pass to the callback.
	
	// Iterate over all the users we requested info for.
	for(int32_t UserIndex = 0; UserIndex < UserIDs.Num(); UserIndex++)
	{
		const EOS_ProductUserId TargetUserID = UserIDs[UserIndex];
		EOS_Connect_GetProductUserExternalAccountCountOptions ExternalAccountCountOptions;
		ExternalAccountCountOptions.ApiVersion = EOS_CONNECT_GETPRODUCTUSEREXTERNALACCOUNTCOUNT_API_LATEST;
		ExternalAccountCountOptions.TargetUserId = TargetUserID;

		FExternalAccountsMap ExternalAccounts;
		const int32_t AccountCount = EOS_Connect_GetProductUserExternalAccountCount(ConnectSubsystem->ConnectHandle, &ExternalAccountCountOptions);

		// Iterate over all the accounts for this user and add them to the ExternalAccounts map.
		for (int32_t AccountIndex = 0; AccountIndex < AccountCount; ++AccountIndex)
		{
			EOS_Connect_CopyProductUserExternalAccountByIndexOptions CopyOptions;
			CopyOptions.ApiVersion = EOS_CONNECT_COPYPRODUCTUSEREXTERNALACCOUNTBYINDEX_API_LATEST;
			CopyOptions.TargetUserId = TargetUserID;
			CopyOptions.ExternalAccountInfoIndex = AccountIndex;

			EOS_Connect_ExternalAccountInfo* EOS_AccountInfo;
			const EOS_EResult Result = EOS_Connect_CopyProductUserExternalAccountByIndex(ConnectSubsystem->ConnectHandle, &CopyOptions, &EOS_AccountInfo);
			
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
				UE_LOG(LogConnectSubsystem, Error, TEXT("Failed to get external account info. Error code: %hs"), EOS_EResult_ToString(Result)); // Handle error.
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
		UUser* OnlineUser = NewObject<UUser>();
		OnlineUser->Initialize(
			TargetUserID,
			'', // TODO: Check if the user has epic external account and set this ID if so.
			ExternalAccounts,
			MostRecentPlatform,
			ExternalAccounts.Contains(MostRecentPlatform) ? ExternalAccounts[MostRecentPlatform].AccountID : "",
			ExternalAccounts.Contains(MostRecentPlatform) ? ExternalAccounts[MostRecentPlatform].DisplayName : "Unknown"
		);
		OnlineUsers.Add(TargetUserID, OnlineUser);
		
		// Load avatar from the user's platform.
		if(OnlineUser->GetPlatform() == EOS_EExternalAccountType::EOS_EAT_STEAM)
		{
			// TODO: this.
			// ConnectSubsystem->UserSubsystem->GetUserAvatar(OnlineUser.PlatformID); // TODO: Create this function which then checks the platform.
		}
		// TODO: Other platforms eventually.
	}
	
	ConnectSubsystem->GetUserInfoCallback(OnlineUsers);
	ConnectSubsystem->GetUserInfoCallback = TFunction<void(FUsersMap)>(); // Clear the callback.
}
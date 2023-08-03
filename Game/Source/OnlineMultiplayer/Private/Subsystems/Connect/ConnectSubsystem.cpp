// Copyright © 2023 Melvin Brink

#include "Subsystems/Connect/ConnectSubsystem.h"
#include "Subsystems/User/Local/LocalUserSubsystem.h"
#include "Subsystems/User/Online/OnlineUserSubsystem.h"
#include "EOSManager.h"
#include "eos_connect.h"
#include "Helpers.h"


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
	LocalUserSubsystem->RequestSteamSessionTicket([this](const std::string& TicketString)
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
	if (!LocalUser)
	{
		UE_LOG(LogConnectSubsystem, Error, TEXT("LocalUser is null."));
		return;
	}
	
	if (Data->ResultCode == EOS_EResult::EOS_Success)
	{
		UE_LOG(LogConnectSubsystem, Log, TEXT("Logged in to Connect-Interface."))
		LocalUser->SetProductUserID(EosProductIDToString(Data->LocalUserId));
		ConnectSubsystem->OnConnectLoginCompleteDelegate.ExecuteIfBound(true, LocalUser);
	}
	else if(Data->ResultCode == EOS_EResult::EOS_InvalidUser)
	{
		// Create a new account. But maybe check if the user wants to do this.
		LocalUser->SetContinuanceToken(Data->ContinuanceToken);
		ConnectSubsystem->CreateNewUser();
	}
	else
	{
		UE_LOG(LogConnectSubsystem, Error, TEXT("LoginConnect failed with error code %hs"), EOS_EResult_ToString(Data->ResultCode));
		ConnectSubsystem->OnConnectLoginCompleteDelegate.ExecuteIfBound(false, nullptr);
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
			UE_LOG(LogConnectSubsystem, Error, TEXT("CreateUser failed with error code %hs"), EOS_EResult_ToString(Data->ResultCode));
			ConnectSubsystem->OnConnectLoginCompleteDelegate.ExecuteIfBound(false, nullptr);
		}
	});
}

void UConnectSubsystem::CheckAccounts()
{
	EOS_Connect_QueryProductUserIdMappingsOptions QueryMappingsOptions = {};
	QueryMappingsOptions.ApiVersion = EOS_CONNECT_QUERYPRODUCTUSERIDMAPPINGS_API_LATEST;
	QueryMappingsOptions.LocalUserId = EosProductIDFromString(LocalUser->GetProductUserID());
	EOS_ProductUserId ProductUserIds[] = {QueryMappingsOptions.LocalUserId};
	QueryMappingsOptions.ProductUserIds = ProductUserIds;
	QueryMappingsOptions.ProductUserIdCount = 1;

	EOS_Connect_QueryProductUserIdMappings(ConnectHandle, &QueryMappingsOptions, this,[](const EOS_Connect_QueryProductUserIdMappingsCallbackInfo* Data)
	{
		if(Data->ResultCode != EOS_EResult::EOS_Success)
		{
			UE_LOG(LogConnectSubsystem, Error, TEXT("QueryProductUserIdMappings failed with error code %hs"), EOS_EResult_ToString(Data->ResultCode));
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

struct FGetUserInfoClientData
{
	UConnectSubsystem* ConnectSubsystem;
	ULocalUserSubsystem* LocalUserSubsystem;
	TArray<EOS_ProductUserId> UserIDs;
	TFunction<void(TArray<UOnlineUser*>)> Callback;
};

/**
 * Tries to get the external-platforms for each Product-User-ID in the given array.
 *
 * @param ProductUserIDList Product-User-IDs used to get the external-platforms of a user.
 * @param Callback The callback to call upon completion
 */
void UConnectSubsystem::GetUserInfo(TArray<FString>& ProductUserIDList, const TFunction<void(TArray<UOnlineUser*> OutUserList)> &Callback)
{
	// Convert array UserIDs of type FString to new array of type EOS_ProductUserId
	TArray<EOS_ProductUserId> ProductUserIDs;
	ProductUserIDs.Reserve(ProductUserIDList.Num());
	for (const FString& ID : ProductUserIDList)
	{
		EOS_ProductUserId ProductUserID = EosProductIDFromString(ID);
		ProductUserIDs.Add(ProductUserID);
	}

	// Properties needed in the callback
	FGetUserInfoClientData* GetUserInfoClientData = new FGetUserInfoClientData();
	GetUserInfoClientData->ConnectSubsystem = this;
	GetUserInfoClientData->UserIDs = ProductUserIDs;
	GetUserInfoClientData->Callback = Callback;

	// Options
	EOS_Connect_QueryProductUserIdMappingsOptions Options;
	Options.ApiVersion = EOS_CONNECT_QUERYPRODUCTUSERIDMAPPINGS_API_LATEST;
	Options.LocalUserId = EosProductIDFromString(LocalUser->GetProductUserID());
	Options.ProductUserIds = ProductUserIDs.GetData();
	Options.ProductUserIdCount = ProductUserIDs.Num();

	// Get the external account mappings from EOS
	EOS_Connect_QueryProductUserIdMappings(ConnectHandle, &Options, GetUserInfoClientData, [](const EOS_Connect_QueryProductUserIdMappingsCallbackInfo* Data)
	{
		// Cast the ClientData back to its type
		const FGetUserInfoClientData* ClientData = static_cast<FGetUserInfoClientData*>(Data->ClientData);
		const UConnectSubsystem* ConnectSubsystem = ClientData->ConnectSubsystem;
		const TArray<EOS_ProductUserId>& UserIDs = ClientData->UserIDs;
		const TFunction<void(TArray<UOnlineUser*> OutUserList)> OnCompleteCallback = ClientData->Callback;
		
		if (Data->ResultCode != EOS_EResult::EOS_Success)
		{
			UE_LOG(LogConnectSubsystem, Error, TEXT("EOS_Connect_QueryProductUserIdMappings failed with error code: [%d]"), Data->ResultCode);
			OnCompleteCallback(TArray<UOnlineUser*>());
			delete ClientData;
			return;
		}
		
		// Iterate over all the users we requested info for and set their properties with the new data
		TArray<UOnlineUser*> OnlineUsers;
		for(int32_t UserIndex = 0; UserIndex < UserIDs.Num(); UserIndex++)
		{
			const EOS_ProductUserId TargetUserID = UserIDs[UserIndex];
			EOS_Connect_GetProductUserExternalAccountCountOptions ExternalAccountCountOptions;
			ExternalAccountCountOptions.ApiVersion = EOS_CONNECT_GETPRODUCTUSEREXTERNALACCOUNTCOUNT_API_LATEST;
			ExternalAccountCountOptions.TargetUserId = TargetUserID;

			// Get the number of ExternalAccounts of this user
			const int32_t AccountCount = EOS_Connect_GetProductUserExternalAccountCount(ConnectSubsystem->ConnectHandle, &ExternalAccountCountOptions);

			// For determining the platform the user is using
			EPlatform MostRecentPlatform = EPlatform::Epic;
			int64_t MostRecentLoginTime = 0;

			// Iterate over all the accounts for this user and add them to this list
			TArray<FPlatformUser> ExternalPlatformUserList;
			for (int32_t AccountIndex = 0; AccountIndex < AccountCount; ++AccountIndex)
			{
				// Options
				EOS_Connect_CopyProductUserExternalAccountByIndexOptions CopyOptions;
				CopyOptions.ApiVersion = EOS_CONNECT_COPYPRODUCTUSEREXTERNALACCOUNTBYINDEX_API_LATEST;
				CopyOptions.TargetUserId = TargetUserID;
				CopyOptions.ExternalAccountInfoIndex = AccountIndex;

				// Get the ExternalAccount data
				EOS_Connect_ExternalAccountInfo* AccountInfo;
				const EOS_EResult Result = EOS_Connect_CopyProductUserExternalAccountByIndex(ConnectSubsystem->ConnectHandle, &CopyOptions, &AccountInfo);
				
				if (Result == EOS_EResult::EOS_Success)
				{
					// Create the platform-user with this received data
					FPlatformUser ExternalPlatformUser;
					ExternalPlatformUser.UserID = FString(AccountInfo->AccountId);
					ExternalPlatformUser.Username = AccountInfo->DisplayName ? AccountInfo->DisplayName : "";
					ExternalPlatformUser.LastLoginTime = AccountInfo->LastLoginTime;

					// Set the platform to an UE friendly enum type
					switch (AccountInfo->AccountIdType)
					{
						case EOS_EExternalAccountType::EOS_EAT_EPIC:
							ExternalPlatformUser.Platform = EPlatform::Epic;
							break;
						case EOS_EExternalAccountType::EOS_EAT_STEAM:
							ExternalPlatformUser.Platform = EPlatform::Epic;
							break;
						case EOS_EExternalAccountType::EOS_EAT_PSN:
							ExternalPlatformUser.Platform = EPlatform::Psn;
							break;
						case EOS_EExternalAccountType::EOS_EAT_XBL:
							ExternalPlatformUser.Platform = EPlatform::Xbox;
							break;
						default:
							ExternalPlatformUser.Platform = EPlatform::Epic;
					}

					// Add this Platform-User to the list
					ExternalPlatformUserList.Add(ExternalPlatformUser);

					// Check if this platform has recently been logged in with, and use the most recent as the user's local-platform (which is not good but there is no alternative)
					if (ExternalPlatformUser.LastLoginTime > MostRecentLoginTime)
					{
						MostRecentLoginTime = ExternalPlatformUser.LastLoginTime;
						MostRecentPlatform = ExternalPlatformUser.Platform;
					}
				}
				else
					UE_LOG(LogConnectSubsystem, Error, TEXT("Failed to get external account info. Error code: [%hs]"), EOS_EResult_ToString(Result));

				// Release the external account data from memory
				EOS_Connect_ExternalAccountInfo_Release(AccountInfo);
			}

			
			// Create the Online-User and set its properties
			UOnlineUser* OnlineUser = NewObject<UOnlineUser>();
			OnlineUser->SetProductUserID(EosProductIDToString(TargetUserID));
			OnlineUser->SetEpicAccountID(FString("")); // TODO: this line

			// Add the external accounts to the ExternalPlatformUserMap on the Online-User, and set the Platform-User to the most recent external account
			TMap<EPlatform, FPlatformUser> ExternalPlatformUserMap;
			for (FPlatformUser PlatformUser : ExternalPlatformUserList)
			{
				ExternalPlatformUserMap.Add(PlatformUser.Platform, PlatformUser);
				if(PlatformUser.Platform == MostRecentPlatform) OnlineUser->SetPlatformUser(PlatformUser);
			}
			OnlineUser->SetExternalPlatformUsers(ExternalPlatformUserMap);

			// Fetch this user's avatar if the platform supports avatars.
			// TODO: This functionality. And load an empty profile picture avatar for the platforms / user's that don't have any.
			// TODO: Call the function on each user, and if the last user's avatar has been fetched, call the callback.
			
			OnlineUsers.Add(OnlineUser);
		}
		
		OnCompleteCallback(OnlineUsers); // TODO: Move to the above TODO
		delete ClientData;
	});
}
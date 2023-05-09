// Copyright © 2023 Melvin Brink

#include "EOS/Subsystems/AuthSubsystem.h"

#include "EOS/EOSManager.h"
#include "eos_auth.h"


void UAuthSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	EosManager = &FEosManager::Get();
	const EOS_HPlatform PlatformHandle = EosManager->GetPlatformHandle();
	if(!PlatformHandle)
	{
		UE_LOG(LogAuthSubsystem, Error, TEXT("Platform-Handle is null"));
		return;
	}
	
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

	FLocalUser& LocalUser = AuthSubsystem->EosManager->GetLocalUser();
	if(Data->ResultCode == EOS_EResult::EOS_Success)
	{
		// Login was successful. We can now use the Auth interface.
		LocalUser.EpicAccountID = Data->LocalUserId;
		LocalUser.bAuthLoggedIn = true;
	}
	else if(Data->ResultCode == EOS_EResult::EOS_Auth_ExternalAuthNotLinked ||
			Data->ResultCode == EOS_EResult::EOS_InvalidUser)
	{
		// Open the login overlay. The user can now login with their Epic account, or create a new one.
		LocalUser.ContinuanceToken = Data->ContinuanceToken;
		LocalUser.bAuthLoggedIn = false;
		AuthSubsystem->LinkUserAuth();
	}
	else
	{
		LocalUser.bAuthLoggedIn = false;
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

	FLocalUser& LocalUser = AuthSubsystem->EosManager->GetLocalUser();
	if (Data->ResultCode == EOS_EResult::EOS_Success)
	{
		LocalUser.ProductUserId = Data->LocalUserId;
		LocalUser.bConnectLoggedIn = true;
	}
	else if(Data->ResultCode == EOS_EResult::EOS_InvalidUser)
	{
		// Create a new account. But maybe check if the user wants to do this.
		LocalUser.ContinuanceToken = Data->ContinuanceToken;
		LocalUser.bConnectLoggedIn = false;
		AuthSubsystem->CreateNewUserConnect();
	}
	else
	{
		LocalUser.bConnectLoggedIn = false;
		UE_LOG(LogAuthSubsystem, Error, TEXT("LoginConnect failed with error code %d"), Data->ResultCode);
	}
}

void UAuthSubsystem::OnLogoutConnectComplete()
{
}


// --------------------------------------------


void UAuthSubsystem::LinkUserAuth()
{
	EOS_Auth_LinkAccountOptions Options;
	Options.ApiVersion = EOS_AUTH_LINKACCOUNT_API_LATEST;
	Options.LinkAccountFlags = EOS_ELinkAccountFlags::EOS_LA_NoFlags;
	Options.LocalUserId = nullptr;
	Options.ContinuanceToken = EosManager->GetLocalUser().ContinuanceToken;
	
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
	EOS_Connect_CreateUserOptions CreateUserOptions;
	CreateUserOptions.ApiVersion = EOS_CONNECT_CREATEUSER_API_LATEST;
	CreateUserOptions.ContinuanceToken = EosManager->GetLocalUser().ContinuanceToken;
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
	EOS_Connect_QueryProductUserIdMappingsOptions QueryMappingsOptions = {};
	QueryMappingsOptions.ApiVersion = EOS_CONNECT_QUERYPRODUCTUSERIDMAPPINGS_API_LATEST;
	QueryMappingsOptions.LocalUserId = EosManager->GetLocalUser().ProductUserId;
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
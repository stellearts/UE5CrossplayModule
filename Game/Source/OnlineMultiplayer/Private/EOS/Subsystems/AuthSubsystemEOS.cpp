// Copyright © 2023 Melvin Brink

#include "EOS/Subsystems/AuthSubsystemEOS.h"

#include "EOS/EOSManager.h"
#include "eos_auth.h"


void UAuthSubsystemEOS::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	EosManager = &FEosManager::Get();
	const EOS_HPlatform PlatformHandle = EosManager->GetPlatformHandle();
	if(!PlatformHandle)
	{
		UE_LOG(LogAuthSubsystemEOS, Error, TEXT("Platform-Handle is null"));
		return;
	}
	
	AuthHandle = EOS_Platform_GetAuthInterface(PlatformHandle);
	ConnectHandle = EOS_Platform_GetConnectInterface(PlatformHandle);

	// LoginAuth();
	LoginConnect();
}


// --------------------------------------------


void UAuthSubsystemEOS::LoginAuth()
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

void UAuthSubsystemEOS::OnLoginAuthComplete(const EOS_Auth_LoginCallbackInfo* Data)
{
	
}

void UAuthSubsystemEOS::LogoutAuth()
{
}

void UAuthSubsystemEOS::OnLogoutAuthComplete()
{
}


// --------------------------------------------


void UAuthSubsystemEOS::LoginConnect()
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

void UAuthSubsystemEOS::LogoutConnect()
{
	
}

void UAuthSubsystemEOS::OnLoginConnectComplete(const EOS_Connect_LoginCallbackInfo* Data)
{
	if (Data->ResultCode == EOS_EResult::EOS_Success)
	{
		// MAY NOT BE NEEDED. THIS CHECKS IF STEAM IS LINKED TO EPIC. BUT STILL SAYS IT IS AFTER UNLINKING.
		// TODO: add to own function
		UAuthSubsystemEOS* AuthSubsystem = static_cast<UAuthSubsystemEOS*>(Data->ClientData);
	
		EOS_Connect_QueryProductUserIdMappingsOptions QueryMappingsOptions = {};
		QueryMappingsOptions.ApiVersion = EOS_CONNECT_QUERYPRODUCTUSERIDMAPPINGS_API_LATEST;
		QueryMappingsOptions.LocalUserId = Data->LocalUserId;
		EOS_ProductUserId ProductUserIds[] = {Data->LocalUserId};
		QueryMappingsOptions.ProductUserIds = ProductUserIds;
		QueryMappingsOptions.ProductUserIdCount = 1;

		EOS_Connect_QueryProductUserIdMappings(AuthSubsystem->ConnectHandle, &QueryMappingsOptions, AuthSubsystem,[](const EOS_Connect_QueryProductUserIdMappingsCallbackInfo* Data)
		{
			if(Data->ResultCode == EOS_EResult::EOS_Success)
			{
				UE_LOG(LogAuthSubsystemEOS, Display, TEXT("QueryProductUserIdMappings success"));

				// TODO: add to own function
				const UAuthSubsystemEOS* AuthSubsystem = static_cast<UAuthSubsystemEOS*>(Data->ClientData);

				EOS_Connect_CopyProductUserExternalAccountByAccountTypeOptions Options = {};
				Options.ApiVersion = EOS_CONNECT_COPYPRODUCTUSEREXTERNALACCOUNTBYACCOUNTTYPE_API_LATEST;
				Options.TargetUserId = Data->LocalUserId;
				Options.AccountIdType = EOS_EExternalAccountType::EOS_EAT_STEAM;

				EOS_Connect_ExternalAccountInfo* ExternalAccountInfo = nullptr;
				EOS_EResult Result = EOS_Connect_CopyProductUserExternalAccountByAccountType(AuthSubsystem->ConnectHandle, &Options, &ExternalAccountInfo);

				if (Result == EOS_EResult::EOS_Success)
				{
					UE_LOG(LogAuthSubsystemEOS, Display, TEXT("Steam account is linked"));
					// Use the ExternalAccountInfo as needed
					// ...
					// Release the memory for ExternalAccountInfo when done
					EOS_Connect_ExternalAccountInfo_Release(ExternalAccountInfo);
				}
				else
				{
					UE_LOG(LogAuthSubsystemEOS, Error, TEXT("Steam account is not linked or an error occurred, error code %d"), Result);
				}
				// end TODO: add to own function
			}
			else
			{
				UE_LOG(LogAuthSubsystemEOS, Error, TEXT("QueryProductUserIdMappings failed with error code %d"), Data->ResultCode);
			}
		});
		// end TODO: add to own function
	}
	else if(Data->ResultCode == EOS_EResult::EOS_InvalidUser)
	{
		// Create a new account. But maybe check if the user wants to do this.
	}
	else
	{
		UE_LOG(LogAuthSubsystemEOS, Error, TEXT("LoginConnect failed with error code %d"), Data->ResultCode);
	}
}

void UAuthSubsystemEOS::OnLogoutConnectComplete()
{
}

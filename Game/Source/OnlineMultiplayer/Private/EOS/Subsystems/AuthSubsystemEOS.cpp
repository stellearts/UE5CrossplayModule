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

	LoginAuth();
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
}

void UAuthSubsystemEOS::LogoutConnect()
{
}

void UAuthSubsystemEOS::OnLoginConnectComplete(const EOS_Connect_LoginCallbackInfo* Data)
{
}

void UAuthSubsystemEOS::OnLogoutConnectComplete()
{
}

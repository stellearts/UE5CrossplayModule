// Copyright © 2023 Melvin Brink

#include "Platforms/EOS/Subsystems/AuthSubsystem.h"
#include "Platforms/EOS/Subsystems/LocalUserSubsystem.h"
#include "Platforms/EOS/EOSManager.h"
#include "eos_auth.h"
#include "Helpers.h"


void UAuthSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Make sure the local user state subsystem is initialized.
	LocalUserSubsystem = Collection.InitializeDependency<ULocalUserSubsystem>();

	EosManager = &FEosManager::Get();
	const EOS_HPlatform PlatformHandle = EosManager->GetPlatformHandle();
	if(!PlatformHandle) return;
	AuthHandle = EOS_Platform_GetAuthInterface(PlatformHandle);
}


// --------------------------------------------


void UAuthSubsystem::Login()
{
	// TODO: Also check if user is already logged in. Would prevent api call.
	if(!AuthHandle) return;
	
	LocalUserSubsystem->RequestSteamSessionTicket([this](const std::string& TicketString)
	{
		EOS_Auth_Credentials Credentials;
		Credentials.ApiVersion = EOS_AUTH_CREDENTIALS_API_LATEST;
		Credentials.Type = EOS_ELoginCredentialType::EOS_LCT_ExternalAuth;
		Credentials.ExternalType = EOS_EExternalCredentialType::EOS_ECT_STEAM_SESSION_TICKET;
		Credentials.Token = TicketString.c_str();
	
		EOS_Auth_LoginOptions Options;
		Options.ApiVersion = EOS_AUTH_LOGIN_API_LATEST;
		Options.Credentials = &Credentials;
		Options.ScopeFlags = EOS_EAuthScopeFlags::EOS_AS_BasicProfile | EOS_EAuthScopeFlags::EOS_AS_FriendsList | EOS_EAuthScopeFlags::EOS_AS_Presence; // This is checked using bitwise operation. Which is why the enums are multiple of 2.
	
		EOS_Auth_Login(AuthHandle, &Options, this, OnLoginComplete);
	});
}

void UAuthSubsystem::OnLoginComplete(const EOS_Auth_LoginCallbackInfo* Data)
{
	UAuthSubsystem* AuthSubsystem = static_cast<UAuthSubsystem*>(Data->ClientData);
	if(!AuthSubsystem) return;
	const ULocalUserSubsystem* LocalUserSubsystem = AuthSubsystem->LocalUserSubsystem;
	
	if(Data->ResultCode == EOS_EResult::EOS_Success)
	{
		// Login was successful. We can now use the Auth interface.
		LocalUserSubsystem->GetLocalUser()->SetEpicAccountID(EosAccountIDToString(Data->LocalUserId));
	}
	else if(Data->ResultCode == EOS_EResult::EOS_Auth_ExternalAuthNotLinked ||
			Data->ResultCode == EOS_EResult::EOS_InvalidUser)
	{
		// Open the login overlay. The user can now login with their Epic account, or create a new one.
		LocalUserSubsystem->GetLocalUser()->SetContinuanceToken(Data->ContinuanceToken);
		AuthSubsystem->LinkUserAuth();
	}
	else
	{
		UE_LOG(LogAuthSubsystem, Error, TEXT("LoginAuth failed with error code %d"), Data->ResultCode);
	}
}

void UAuthSubsystem::Logout()
{
}

void UAuthSubsystem::OnLogoutComplete()
{
}


// --------------------------------------------


void UAuthSubsystem::LinkUserAuth()
{
	EOS_Auth_LinkAccountOptions Options;
	Options.ApiVersion = EOS_AUTH_LINKACCOUNT_API_LATEST;
	Options.LinkAccountFlags = EOS_ELinkAccountFlags::EOS_LA_NoFlags;
	Options.LocalUserId = nullptr;
	Options.ContinuanceToken = LocalUserSubsystem->GetLocalUser()->GetContinuanceToken();
	
	EOS_Auth_LinkAccount(AuthHandle, &Options, this, [](const EOS_Auth_LinkAccountCallbackInfo* Data)
	{
		UAuthSubsystem* AuthSubsystem = static_cast<UAuthSubsystem*>(Data->ClientData);
		if(!AuthSubsystem) return;
		
		if(Data->ResultCode == EOS_EResult::EOS_Success)
		{
			UE_LOG(LogAuthSubsystem, Display, TEXT("LinkAccount success"));
			AuthSubsystem->Login();
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
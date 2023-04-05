// Copyright © 2023 Melvin Brink


#include "Multiplayer/SessionManager.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "AdvancedSessions.h"
#include "AdvancedSessionsLibrary.h"
#include "CreateSessionCallbackProxyAdvanced.h"


USessionManager::USessionManager():
	OnCreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete)),
	OnJoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete)),
	OnFindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsComplete)),
	OnDestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete)),
	OnStartSessionCompleteDelegate(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartSessionComplete))
{
	if (const IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get()) SessionInterface = Subsystem->GetSessionInterface();
}



void USessionManager::CreateSession()
{
	if (!SessionInterface) return;

	// Destroy current session if it exists.
	if (auto const ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession); ExistingSession != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Existing session found"));
		bCreateSessionOnDestroy = true;
		DestroySession();
		return;
	}
	
	// log subsystem name
	FString SubsystemName = IOnlineSubsystem::Get()->GetSubsystemName().ToString();
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Online Subsystem: %s"), *SubsystemName));
	
	LastSessionSettings = MakeShared<FOnlineSessionSettings>();
	LastSessionSettings->bIsLANMatch = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL";
	LastSessionSettings->bShouldAdvertise = IOnlineSubsystem::Get()->GetSubsystemName() != "NULL";
	UE_LOG(LogTemp, Warning, TEXT("Is LAN match: %s"), LastSessionSettings->bIsLANMatch ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Warning, TEXT("Should advertise: %s"), LastSessionSettings->bShouldAdvertise ? TEXT("true") : TEXT("false"));
	
	LastSessionSettings->NumPublicConnections = 4;
	LastSessionSettings->bUsesPresence = true;
	LastSessionSettings->bAllowJoinViaPresence = true;
	LastSessionSettings->BuildUniqueId = 1;
	LastSessionSettings->bAllowJoinInProgress = true;
	
	
	
	OnCreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegate);
	if (!SessionInterface->CreateSession(*GetWorld()->GetFirstLocalPlayerFromController()->GetPreferredUniqueNetId(), NAME_GameSession, *LastSessionSettings))
	{
		ManagerOnStartSessionComplete.Broadcast(false);
	}
}


void USessionManager::JoinSession(const FOnlineSessionSearchResult& SessionResult)
{
	if(!SessionInterface.IsValid())
	{
		ManagerOnJoinSessionComplete.Broadcast(FString(), EOnJoinSessionCompleteResult::UnknownError);
		return;
	}
	FString SubsystemName = IOnlineSubsystem::Get()->GetSubsystemName().ToString();
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Online Subsystem: %s"), *SubsystemName));

	OnJoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if(!SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SessionResult))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);
		ManagerOnJoinSessionComplete.Broadcast(FString(), EOnJoinSessionCompleteResult::UnknownError);
	}
}


void USessionManager::FindSessions()
{
	if(!SessionInterface.IsValid()) return;
	UE_LOG(LogTemp, Warning, TEXT("USessionManager::FindSessions()"));
	
	// log subsystem name
	FString SubsystemName = IOnlineSubsystem::Get()->GetSubsystemName().ToString();
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Online Subsystem: %s"), *SubsystemName));

	LastSessionSearch = MakeShared<FOnlineSessionSearch>();
	LastSessionSearch->MaxSearchResults = 10000;
	LastSessionSearch->bIsLanQuery = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL";
	LastSessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
	
	OnFindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegate);
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if(!SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);
		ManagerOnFindSessionsComplete.Broadcast(false, TArray<FOnlineSessionSearchResult>());
	}
}


void USessionManager::DestroySession()
{
	if(!SessionInterface.IsValid())
	{
		ManagerOnDestroySessionComplete.Broadcast(false);
		return;
	}

	OnDestroySessionCompleteHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegate);
	if(!SessionInterface->DestroySession(NAME_GameSession))
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteHandle);
		ManagerOnDestroySessionComplete.Broadcast(false);
	}
}


void USessionManager::StartSession()
{
}



/*
	Callbacks
 */ 

void USessionManager::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface) SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);
	ManagerOnCreateSessionComplete.Broadcast(bWasSuccessful);
}


void USessionManager::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (SessionInterface) SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);
	FString Address;
	SessionInterface->GetResolvedConnectString(NAME_GameSession, Address);
	ManagerOnJoinSessionComplete.Broadcast(Address, Result);
}


void USessionManager::OnFindSessionsComplete(bool bWasSuccessful)
{
	if(SessionInterface) SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);
	ManagerOnFindSessionsComplete.Broadcast(bWasSuccessful, LastSessionSearch->SearchResults);
}


void USessionManager::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if(SessionInterface) SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteHandle);
	ManagerOnDestroySessionComplete.Broadcast(bWasSuccessful);

	// Create a new session if we were trying to create one before destroying the old one.
	if(bWasSuccessful && bCreateSessionOnDestroy)
	{
		bCreateSessionOnDestroy = false;
		CreateSession();
	}
}


void USessionManager::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
}
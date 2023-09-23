// Copyright © 2023 Melvin Brink

#include "LatentAction/StartListenServer.h"
#include "Subsystems/Lobby/LobbySubsystem.h"
#include "Subsystems/Session/SessionSubsystem.h"
#include "HttpModule.h"
#include "GameModes/MultiplayerGameMode.h"
#include "Interfaces/IHttpResponse.h"
#include "PlayerStates/MultiplayerPlayerState.h"


UStartListenServer::UStartListenServer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UStartListenServer* UStartListenServer::StartListenServer(UObject* WorldContextObject)
{
	UStartListenServer* Proxy = NewObject<UStartListenServer>();
	Proxy->World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	return Proxy;
}

void UStartListenServer::Activate()
{
	// Check if using correct GameMode.
	const AMultiplayerGameMode* MultiplayerGameMode = Cast<AMultiplayerGameMode>(World->GetAuthGameMode());
	if(!MultiplayerGameMode)
	{
		UE_LOG(LogTemp, Warning, TEXT("GameMode class that is in use should be derived from 'AMultiplayerGameMode' when calling StartListenServer"));
		OnFailure.Broadcast(); // todo error message
		return;
	}
	
	FHttpModule* Http = &FHttpModule::Get();
	if(!Http || !Http->IsHttpEnabled())
	{
		OnFailure.Broadcast(); // todo error message
		return;
	}

	// Get public address.
	const auto Request = Http->CreateRequest();
	Request->SetURL("http://api.ipify.org");
	Request->SetHeader("Content-Type" ,"text/html");
	Request->SetTimeout(10); // todo, enum state member variable so that this can't get called multiple times while a request is busy.
	Request->OnProcessRequestComplete().BindUObject(this, &UStartListenServer::OnHttpRequestCompleted);
	
	if (!Request->ProcessRequest())
	{
		OnFailure.Broadcast(); // todo error message
	}
}

void UStartListenServer::OnHttpRequestCompleted(TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> Request, TSharedPtr<IHttpResponse, ESPMode::ThreadSafe> Response, const bool bWasSuccessful)
{
	if(!bWasSuccessful)
	{
		UE_LOG(LogTemp, Error, TEXT("UStartListenServer::OnHttpRequestCompleted has failed with an unknown error."));
		OnFailure.Broadcast();
		return;
	}
	
	ResponseString = Response.Get()->GetContentAsString();
	if(ResponseString.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("UStartListenServer::OnHttpRequestCompleted has returned an empty ResponseString for the Server-Address."));
		OnFailure.Broadcast();
		return;
	}
	
	StartServerCompleteDelegateHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::ServerStarted);
	World->ServerTravel("/Game/Maps/MainMenu?listen");
}

void UStartListenServer::ServerStarted(UWorld* NewWorld)
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(StartServerCompleteDelegateHandle);

	// We loaded into a copy of the level we were in, so the world is also different now.
	World = NewWorld;

	UE_LOG(LogTemp, Log, TEXT("Game server has started."));
	
	const UGameInstance* GameInstance = NewWorld->GetGameInstance();
	ULobbySubsystem* LobbySubsystem = GameInstance->GetSubsystem<ULobbySubsystem>();
	
	if(LobbySubsystem->ActiveLobby())
	{
		// If only in a lobby, then set the address attribute on the lobby so that they can join.
		UE_LOG(LogTemp, Log, TEXT("Requesting lobby members to join server..."));
	
		// Set public address attribute on lobby.
		FLobbyAttribute ServerAddressAttribute;
		ServerAddressAttribute.Key = "ServerAddress";
		ServerAddressAttribute.Type = ELobbyAttributeType::String;
		ServerAddressAttribute.StringValue = ResponseString;
		LobbySubsystem->SetAttribute(ServerAddressAttribute, [this, NewWorld, LobbySubsystem](const bool bSuccess)
		{
			if(bSuccess)
			{
				WaitForPlayersToJoin(LobbySubsystem);
			}
			else
			{
				// Stop hosting.
				UE_LOG(LogTemp, Error, TEXT("Failed to set the 'ServerAddressAttribute' on the lobby. Players cannot join without it."));
				StopServer();
			}
		});
	}
	else
	{
		OnSuccess.Broadcast();
	}
}

void UStartListenServer::WaitForPlayersToJoin(ULobbySubsystem* LobbySubsystem)
{
	const TArray<UOnlineUser*> LobbyMembers = LobbySubsystem->GetLobby().GetMemberList();

	if(!LobbyMembers.Num())
	{
		UE_LOG(LogTemp, Log, TEXT("No players in lobby, starting game alone."));
		OnSuccess.Broadcast();
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Waiting for members to join..."));

	// Retrieve the local PlayerState and attempt to cast it to AMultiplayerPlayerState
	APlayerState* LocalPlayerState = World->GetFirstPlayerController()->PlayerState;
	World->GetFirstPlayerController()->PlayerState;

	if(AMultiplayerPlayerState* MultiplayerPlayerState = Cast<AMultiplayerPlayerState>(LocalPlayerState))
	{
		JoinedMembers.Empty();
		
		// Bind to the delegate
		OnPlayerProductIDSetDelegateHandle = MultiplayerPlayerState->OnPlayerProductIDSetDelegate.AddLambda([&, MultiplayerPlayerState](const FString& ProductUserID)
		{
			JoinedMembers.Add(ProductUserID);

			// todo also check if Lobby member(s) have left during this async operation.
			if(JoinedMembers.Num() == LobbyMembers.Num())
			{
				UE_LOG(LogTemp, Log, TEXT("All members have joined the game server successfully."));
				MultiplayerPlayerState->OnPlayerProductIDSetDelegate.Remove(OnPlayerProductIDSetDelegateHandle);
				
				World->GetTimerManager().ClearTimer(TimeoutTimerHandle);
				OnSuccess.Broadcast();
			}
		});

		// Timer for a timeout failure case
		World->GetTimerManager().SetTimer(TimeoutTimerHandle, [&, MultiplayerPlayerState]()
		{
			UE_LOG(LogTemp, Log, TEXT("Failed to start server because some members failed to join."));
			MultiplayerPlayerState->OnPlayerProductIDSetDelegate.Remove(OnPlayerProductIDSetDelegateHandle);
			StopServer();
			
		}, TimeoutDuration, false);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("PlayerState used is not of type 'AMultiplayerPlayerState', please derive your custom PlayerState from this base class."));
		StopServer();
	}
}

/*
 * When some process has failed during the creation of a server, we need to stop the server and broadcast a failure.
 */
void UStartListenServer::StopServer()
{
	StopServerCompleteDelegateHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::ServerStopped);
	if(APlayerController* PlayerController = World->GetFirstPlayerController(); PlayerController) PlayerController->ClientTravel(TEXT("MainMenu"), TRAVEL_Absolute);
}

void UStartListenServer::ServerStopped(UWorld* NewWorld)
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(StopServerCompleteDelegateHandle);

	// Stopped the server and once again moved to a copy of the level we were in, so set the world again.
	World = NewWorld;
	OnFailure.Broadcast();
}

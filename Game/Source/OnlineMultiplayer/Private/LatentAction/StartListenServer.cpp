// Copyright © 2023 Melvin Brink

#include "LatentAction/StartListenServer.h"
#include "Subsystems/Lobby/LobbySubsystem.h"
#include "Subsystems/Session/SessionSubsystem.h"
#include "HttpModule.h"
#include "GameModes/MultiplayerGameMode.h"
#include "Interfaces/IHttpResponse.h"


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
		UE_LOG(LogLobbySubsystem, Warning, TEXT("GameMode class that is in use should be derived from 'AMultiplayerGameMode' when calling StartListenServer"));
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
		OnFailure.Broadcast();
		return;
	}
	
	ResponseString = Response.Get()->GetContentAsString();
	if(ResponseString.IsEmpty())
	{
		OnFailure.Broadcast();
		return;
	}
	
	StartServerCompleteDelegateHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddLambda([this](const UWorld* World)
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(StartServerCompleteDelegateHandle);
	
		const UGameInstance* GameInstance = World->GetGameInstance();
		ULobbySubsystem* LobbySubsystem = GameInstance->GetSubsystem<ULobbySubsystem>();
		const USessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<USessionSubsystem>();
	
		// If in a lobby or session, wait until all members have joined first before broadcasting success.
		if(SessionSubsystem->ActiveSession())
		{
			// If in session, then let members know that they can join the server. Lobby members should join session in this case.
			UE_LOG(LogLobbySubsystem, Warning, TEXT("Requesting session members to join server..."));

			// todo: make players in session know that they can join.
			// todo this will come later if needed, not required for p2p but for now ill keep it here.
			
			OnSuccess.Broadcast();
		}
		else if(LobbySubsystem->ActiveLobby())
		{
			// If only in a lobby, then set the address attribute on the lobby so that they can join.
			UE_LOG(LogLobbySubsystem, Warning, TEXT("Requesting lobby members to join server..."));
	
			// Set public address attribute on lobby.
			FLobbyAttribute ServerAddressAttribute;
			ServerAddressAttribute.Key = "ServerAddress";
			ServerAddressAttribute.Type = ELobbyAttributeType::String;
			ServerAddressAttribute.StringValue = ResponseString;
			LobbySubsystem->SetAttribute(ServerAddressAttribute, [this, World, LobbySubsystem](const bool bSuccess)
			{
				if(bSuccess)
				{
					const TArray<UOnlineUser*> LobbyMembers = LobbySubsystem->GetLobby().GetMemberList();
					const TArray<UOnlineUser*> JoinedMembers;

					UE_LOG(LogLobbySubsystem, Warning, TEXT("Waiting for members to join..."));
					// Check for people joining unreal server here...
					// todo: player-controller joins server, player that join's sets their product-user-id variable that gets replicated to server and validate.
					
					OnSuccess.Broadcast();
				}
				else
				{
					OnFailure.Broadcast();
					APlayerController* PlayerController = World->GetFirstPlayerController();
					if(PlayerController) PlayerController->ClientTravel(TEXT("MainMenu"), TRAVEL_Absolute);
				}
			});
		}
		else
		{
			OnSuccess.Broadcast();
		}
	});

	World->ServerTravel("?listen");
}

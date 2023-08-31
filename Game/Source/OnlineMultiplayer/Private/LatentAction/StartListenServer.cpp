// Copyright © 2023 Melvin Brink

#include "LatentAction/StartListenServer.h"
#include "Subsystems/Lobby/LobbySubsystem.h"
#include "Subsystems/Session/SessionSubsystem.h"
#include "HttpModule.h"
#include "GameFramework/GameModeBase.h"
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
	// todo
	
	// Get public address for server.
	FHttpModule* Http = &FHttpModule::Get();
	if(!Http || !Http->IsHttpEnabled())
	{
		OnFailure.Broadcast(); // todo error message
		return;
	}
	
	const auto Request = Http->CreateRequest();
	Request->SetURL("http://api.ipify.org");
	Request->SetHeader("Content-Type" ,"text/html");
	Request->SetTimeout(3);

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
	
	FString ResponseString = Response.Get()->GetContentAsString();
	if(ResponseString.IsEmpty())
	{
		OnFailure.Broadcast();
		return;
	}
	
	StartServerCompleteDelegateHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddLambda([this, ResponseString](const UWorld* World)
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
			LobbySubsystem->SetAttribute(ServerAddressAttribute, [this, World](const bool bSuccess)
			{
				if(bSuccess)
				{
					const TArray<UOnlineUser*> LobbyMembers;

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

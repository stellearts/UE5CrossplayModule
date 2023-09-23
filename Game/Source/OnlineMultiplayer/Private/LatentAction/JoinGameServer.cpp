// Copyright © 2023 Melvin Brink

#include "LatentAction/JoinGameServer.h"


UJoinGameServer::UJoinGameServer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UJoinGameServer* UJoinGameServer::JoinGameServer(FString ServerAddress, APlayerController* PlayerController)
{
	UJoinGameServer* Proxy = NewObject<UJoinGameServer>();
	Proxy->ServerAddress = ServerAddress;
	Proxy->PlayerController = PlayerController;
	return Proxy;
}

void UJoinGameServer::Activate()
{
	if(!PlayerController || ServerAddress.IsEmpty())
	{
		OnFailure.Broadcast();
		return;
	}

	JoinServerCompleteDelegateHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::ServerJoined);
	PlayerController->ClientTravel(ServerAddress + ":7777", TRAVEL_Absolute);
	OnSuccess.Broadcast();
}

void UJoinGameServer::ServerJoined(UWorld* NewWorld)
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(JoinServerCompleteDelegateHandle);

	// We loaded into a copy of the level we were in, so the world is also different now.
	World = NewWorld;
	UE_LOG(LogTemp, Warning, TEXT("Joined the game server."));
}

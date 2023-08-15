// Copyright © 2023 Melvin Brink

#include "Subsystems/Lobby/CallbackProxies/CreateLobbyCallbackProxy.h"
#include "Subsystems/Lobby/LobbySubsystem.h"



UCreateLobbyCallbackProxy::UCreateLobbyCallbackProxy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UCreateLobbyCallbackProxy* UCreateLobbyCallbackProxy::CreateLobby(UObject* WorldContextObject, const int32 MaxMembers)
{
	UCreateLobbyCallbackProxy* Proxy = NewObject<UCreateLobbyCallbackProxy>();
	Proxy->WorldContextObject = WorldContextObject;
	Proxy->MaxMembers = MaxMembers;
	return Proxy;
}

void UCreateLobbyCallbackProxy::Activate()
{
	ULobbySubsystem* LobbySubsystem = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull)->GetGameInstance()->GetSubsystem<ULobbySubsystem>();
	if(LobbySubsystem)
	{
		CreateLobbyDelegateHandle = LobbySubsystem->OnCreateLobbyCompleteDelegate.AddLambda([this, LobbySubsystem](const ECreateLobbyResultCode LobbyResultCode, const FLobbyDetails& LobbyDetails)
		{
			LobbySubsystem->OnCreateLobbyCompleteDelegate.Remove(CreateLobbyDelegateHandle);
			
			FCreateLobbyResult Result;
			Result.LobbyResultCode = LobbyResultCode;
			Result.LobbyDetails = LobbyDetails;

			if(LobbyResultCode == ECreateLobbyResultCode::Success) OnSuccess.Broadcast(Result);
			else OnFailure.Broadcast(Result);
		});
		LobbySubsystem->CreateLobby(MaxMembers);
	}
	else
	{
		FCreateLobbyResult Result;
		Result.LobbyResultCode = ECreateLobbyResultCode::Unknown;
		OnFailure.Broadcast(Result);
	}
}

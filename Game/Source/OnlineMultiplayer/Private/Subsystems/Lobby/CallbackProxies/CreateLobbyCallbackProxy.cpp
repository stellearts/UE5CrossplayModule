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
		// Bind the delegate to the callback function
		FDelegateHandle Handle;
		auto Callback = [this, LobbySubsystem, &Handle](const ELobbyResultCode LobbyResultCode, const FLobbyDetails& LobbyDetails)
		{
			LobbySubsystem->OnCreateLobbyCompleteDelegate.Remove(Handle);
			
			FCreateLobbyResult Result;
			Result.LobbyResultCode = LobbyResultCode;
			Result.LobbyDetails = LobbyDetails;

			if(LobbyResultCode == ELobbyResultCode::Success) OnSuccess.Broadcast(Result);
			else OnFailure.Broadcast(Result);
		};
		Handle = LobbySubsystem->OnCreateLobbyCompleteDelegate.AddLambda(Callback);

		// Create a lobby which will call the callback when done
		LobbySubsystem->CreateLobby(MaxMembers);
	}
	else
	{
		FCreateLobbyResult Result;
		Result.LobbyResultCode = ELobbyResultCode::Unknown;
		OnFailure.Broadcast(Result);
	}
}

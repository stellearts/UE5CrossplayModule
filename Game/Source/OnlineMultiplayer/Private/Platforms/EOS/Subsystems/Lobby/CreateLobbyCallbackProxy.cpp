// Copyright © 2023 Melvin Brink

#include "Platforms/EOS/Subsystems/Lobby/CreateLobbyCallbackProxy.h"
#include "Platforms/EOS/Subsystems/Lobby/LobbySubsystem.h"



UCreateLobbyCallbackProxy::UCreateLobbyCallbackProxy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UCreateLobbyCallbackProxy* UCreateLobbyCallbackProxy::CreateLobby(UObject* WorldContextObject)
{
	UCreateLobbyCallbackProxy* Proxy = NewObject<UCreateLobbyCallbackProxy>();
	Proxy->WorldContextObject = WorldContextObject;
	return Proxy;
}

void UCreateLobbyCallbackProxy::Activate()
{
	ULobbySubsystem* LobbySubsystem = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull)->GetGameInstance()->GetSubsystem<ULobbySubsystem>();
	
	if(LobbySubsystem)
	{
		
		
		// Bind the delegate to the callback function
		FDelegateHandle Handle;
		auto Lambda = [this, LobbySubsystem, &Handle](ELobbyResultCode LobbyResultCode, const FLobbyDetails& LobbyDetails)
		{
			LobbySubsystem->OnCreateLobbyCompleteDelegate.Remove(Handle);
			
			FCreateLobbyResult Result;
			Result.LobbyResultCode = LobbyResultCode;
			Result.LobbyDetails = LobbyDetails;

			if(LobbyResultCode == ELobbyResultCode::Success) OnSuccess.Broadcast(Result);
			else OnFailure.Broadcast(Result);
		};
		Handle = LobbySubsystem->OnCreateLobbyCompleteDelegate.AddLambda(Lambda);

		// Create a lobby which will call the above lambda when done
		LobbySubsystem->CreateLobby();
	}
	else
	{
		FCreateLobbyResult Result;
		Result.LobbyResultCode = ELobbyResultCode::Unknown;
		OnFailure.Broadcast(Result);
	}
}

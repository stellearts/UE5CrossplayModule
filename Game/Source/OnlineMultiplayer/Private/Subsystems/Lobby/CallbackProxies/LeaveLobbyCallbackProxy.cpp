// Copyright © 2023 Melvin Brink

#include "Subsystems/Lobby/CallbackProxies/LeaveLobbyCallbackProxy.h"
#include "Subsystems/Lobby/LobbySubsystem.h"



ULeaveLobbyCallbackProxy::ULeaveLobbyCallbackProxy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

ULeaveLobbyCallbackProxy* ULeaveLobbyCallbackProxy::LeaveLobby(UObject* WorldContextObject, const int32 MaxMembers)
{
	ULeaveLobbyCallbackProxy* Proxy = NewObject<ULeaveLobbyCallbackProxy>();
	Proxy->WorldContextObject = WorldContextObject;
	return Proxy;
}

void ULeaveLobbyCallbackProxy::Activate()
{
	ULobbySubsystem* LobbySubsystem = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull)->GetGameInstance()->GetSubsystem<ULobbySubsystem>();
	if(LobbySubsystem)
	{
		// Bind the delegate to the callback function
		FDelegateHandle Handle;
		auto Callback = [this, LobbySubsystem, &Handle](const ELobbyResultCode LobbyResultCode)
		{
			LobbySubsystem->OnLeaveLobbyCompleteDelegate.Remove(Handle);
			if(LobbyResultCode == ELobbyResultCode::Success) OnSuccess.Broadcast(LobbyResultCode);
			else OnFailure.Broadcast(LobbyResultCode);
		};
		Handle = LobbySubsystem->OnLeaveLobbyCompleteDelegate.AddLambda(Callback);

		// Leave the lobby
		LobbySubsystem->LeaveLobby();
	}
	else OnFailure.Broadcast(ELobbyResultCode::Unknown);
}

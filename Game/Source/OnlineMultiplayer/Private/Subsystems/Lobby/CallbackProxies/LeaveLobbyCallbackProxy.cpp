// Copyright © 2023 Melvin Brink

#include "Subsystems/Lobby/CallbackProxies/LeaveLobbyCallbackProxy.h"
#include "Subsystems/Lobby/LobbySubsystem.h"



ULeaveLobbyCallbackProxy::ULeaveLobbyCallbackProxy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

ULeaveLobbyCallbackProxy* ULeaveLobbyCallbackProxy::LeaveLobby(UObject* WorldContextObject)
{
	ULeaveLobbyCallbackProxy* Proxy = NewObject<ULeaveLobbyCallbackProxy>();
	Proxy->WorldContextObject = WorldContextObject;
	return Proxy;
}

void ULeaveLobbyCallbackProxy::Activate()
{
	if(!LobbySubsystem) LobbySubsystem = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull)->GetGameInstance()->GetSubsystem<ULobbySubsystem>();
	if(LobbySubsystem->InLobby())
	{
		LeaveLobbyDelegateHandle = LobbySubsystem->OnLeaveLobbyCompleteDelegate.AddUObject(this, &ThisClass::OnCompleted);
		LobbySubsystem->LeaveLobby();
	}
	else OnSuccess.Broadcast(ELobbyResultCode::Success); // Since we are not in a lobby, just return a success.
}

void ULeaveLobbyCallbackProxy::OnCompleted(const ELobbyResultCode LobbyResultCode)
{
	if(LobbySubsystem) LobbySubsystem->OnLeaveLobbyCompleteDelegate.Remove(LeaveLobbyDelegateHandle);
	if(LobbyResultCode == ELobbyResultCode::Success) OnSuccess.Broadcast(LobbyResultCode);
	else OnFailure.Broadcast(LobbyResultCode);
}
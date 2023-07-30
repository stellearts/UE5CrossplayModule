// Copyright © 2023 Melvin Brink

#include "Platforms/EOS/Subsystems/Lobby/JoinLobbyCallbackProxy.h"
#include "Platforms/EOS/Subsystems/Lobby/LobbySubsystem.h"



UJoinLobbyCallbackProxy::UJoinLobbyCallbackProxy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UJoinLobbyCallbackProxy* UJoinLobbyCallbackProxy::JoinLobbyByID(UObject* WorldContextObject, FString LobbyID)
{
	UJoinLobbyCallbackProxy* Proxy = NewObject<UJoinLobbyCallbackProxy>();
	Proxy->LobbyID = LobbyID;
	Proxy->WorldContextObject = WorldContextObject;
	return Proxy;
}

void UJoinLobbyCallbackProxy::Activate()
{
	ULobbySubsystem* LobbySubsystem = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull)->GetGameInstance()->GetSubsystem<ULobbySubsystem>();

	FJoinLobbyResult Result;
	if (!LobbyID.IsEmpty())
	{
		Result.MemberList = LobbySubsystem->GetMemberList();
		Result.ErrorMessage = FText::FromString("No error");
		OnSuccess.Broadcast(Result);
	}
	else
	{
        Result.ErrorMessage = FText::FromString("Test error");
        OnFailure.Broadcast(Result);
	}
}

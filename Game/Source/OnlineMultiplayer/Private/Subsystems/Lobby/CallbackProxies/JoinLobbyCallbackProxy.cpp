// Copyright © 2023 Melvin Brink

#include "Subsystems/Lobby/CallbackProxies/JoinLobbyCallbackProxy.h"
#include "Subsystems/User/Local/LocalUserSubsystem.h"
#include "Subsystems/Lobby/LobbySubsystem.h"



// ------------------------------ Start Join Lobby by Lobby ID ------------------------------

UJoinLobbyByLobbyIDCallbackProxy::UJoinLobbyByLobbyIDCallbackProxy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UJoinLobbyByLobbyIDCallbackProxy* UJoinLobbyByLobbyIDCallbackProxy::JoinLobbyByID(UObject* WorldContextObject, FString LobbyID)
{
	UJoinLobbyByLobbyIDCallbackProxy* Proxy = NewObject<UJoinLobbyByLobbyIDCallbackProxy>();
	Proxy->LobbyID = LobbyID;
	Proxy->WorldContextObject = WorldContextObject;
	return Proxy;
}

void UJoinLobbyByLobbyIDCallbackProxy::Activate()
{
	ULobbySubsystem* LobbySubsystem = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull)->GetGameInstance()->GetSubsystem<ULobbySubsystem>();
	if (!LobbyID.IsEmpty())
	{
		JoinLobbyDelegateHandle = LobbySubsystem->OnJoinLobbyCompleteDelegate.AddLambda([this, LobbySubsystem](const EJoinLobbyResultCode LobbyResultCode, const FLobbyDetails& LobbyDetails)
		{
			LobbySubsystem->OnJoinLobbyCompleteDelegate.Remove(JoinLobbyDelegateHandle);

			FJoinLobbyResult Result;
			Result.LobbyResultCode = LobbyResultCode;
			Result.LobbyDetails = LobbyDetails;

			if(LobbyResultCode == EJoinLobbyResultCode::Success) OnSuccess.Broadcast(Result);
			else OnFailure.Broadcast(Result);
		});
		LobbySubsystem->JoinLobbyByID(LobbyID);
	}
	else
	{
		FJoinLobbyResult Result;
        Result.LobbyResultCode = EJoinLobbyResultCode::InvalidLobbyID;
        OnFailure.Broadcast(Result);
	}
}

// ------------------------------ End Join Lobby by Lobby ID ------------------------------



// ------------------------------ Start Join Lobby by User ID ------------------------------

UJoinLobbyByUserIDCallbackProxy::UJoinLobbyByUserIDCallbackProxy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UJoinLobbyByUserIDCallbackProxy* UJoinLobbyByUserIDCallbackProxy::JoinLobbyByUserID(UObject* WorldContextObject, FString UserID)
{
	UJoinLobbyByUserIDCallbackProxy* Proxy = NewObject<UJoinLobbyByUserIDCallbackProxy>();
	Proxy->UserID = UserID;
	Proxy->WorldContextObject = WorldContextObject;
	return Proxy;
}

void UJoinLobbyByUserIDCallbackProxy::Activate()
{
	// TODO: Change handle to member variable delegate, also for createlobbycallbackproxy.
	ULobbySubsystem* LobbySubsystem = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull)->GetGameInstance()->GetSubsystem<ULobbySubsystem>();
	if (!UserID.IsEmpty() || !LobbySubsystem)
	{
		JoinLobbyDelegateHandle = LobbySubsystem->OnJoinLobbyCompleteDelegate.AddLambda([this, LobbySubsystem](const EJoinLobbyResultCode LobbyResultCode, const FLobbyDetails& LobbyDetails)
		{
			LobbySubsystem->OnJoinLobbyCompleteDelegate.Remove(JoinLobbyDelegateHandle);

			FJoinLobbyResult Result;
			Result.LobbyResultCode = LobbyResultCode;
			Result.LobbyDetails = LobbyDetails;

			if(LobbyResultCode == EJoinLobbyResultCode::Success) OnSuccess.Broadcast(Result);
			else OnFailure.Broadcast(Result);
		});
		LobbySubsystem->JoinLobbyByUserID(UserID);
	}
	else
	{
		FJoinLobbyResult Result;
		Result.LobbyResultCode = EJoinLobbyResultCode::InvalidUserID;
		OnFailure.Broadcast(Result);
	}
}

// ------------------------------ End Join Lobby by User ID ------------------------------
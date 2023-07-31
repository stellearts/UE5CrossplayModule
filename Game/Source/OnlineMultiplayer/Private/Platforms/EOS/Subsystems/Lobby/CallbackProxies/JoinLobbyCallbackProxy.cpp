// Copyright © 2023 Melvin Brink

#include "Platforms/EOS/Subsystems/Lobby/CallbackProxies/JoinLobbyCallbackProxy.h"

#include "Platforms/EOS/Subsystems/LocalUserSubsystem.h"
#include "Platforms/EOS/Subsystems/Lobby/LobbySubsystem.h"



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
	if (!LobbyID.IsEmpty() || !LobbySubsystem)
	{
		FDelegateHandle Handle;
		auto Callback = [this, LobbySubsystem, &Handle](const ELobbyResultCode LobbyResultCode, const FLobbyDetails& LobbyDetails)
		{
			LobbySubsystem->OnJoinLobbyCompleteDelegate.Remove(Handle);

			FJoinLobbyResult Result;
			Result.LobbyResultCode = LobbyResultCode;
			Result.LobbyDetails = LobbyDetails;

			if(LobbyResultCode == ELobbyResultCode::Success) OnSuccess.Broadcast(Result);
			else OnFailure.Broadcast(Result);
		};
		Handle = LobbySubsystem->OnJoinLobbyCompleteDelegate.AddLambda(Callback);

		// Try to join a lobby using the ID which will call the callback when done
		LobbySubsystem->JoinLobbyByID(LobbyID);
	}
	else
	{
		FJoinLobbyResult Result;
        Result.LobbyResultCode = ELobbyResultCode::InvalidLobbyID;
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
	ULobbySubsystem* LobbySubsystem = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull)->GetGameInstance()->GetSubsystem<ULobbySubsystem>();
	ULocalUserSubsystem* LocalUserSubsystem = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull)->GetGameInstance()->GetSubsystem<ULocalUserSubsystem>();
	UserID = LocalUserSubsystem->GetLocalUser()->GetProductUserID();
	if (!UserID.IsEmpty() || !LobbySubsystem)
	{
		FDelegateHandle Handle;
		auto Callback = [this, LobbySubsystem, &Handle](const ELobbyResultCode LobbyResultCode, const FLobbyDetails& LobbyDetails)
		{
			LobbySubsystem->OnJoinLobbyCompleteDelegate.Remove(Handle);

			FJoinLobbyResult Result;
			Result.LobbyResultCode = LobbyResultCode;
			Result.LobbyDetails = LobbyDetails;

			if(LobbyResultCode == ELobbyResultCode::Success) OnSuccess.Broadcast(Result);
			else OnFailure.Broadcast(Result);
		};
		Handle = LobbySubsystem->OnJoinLobbyCompleteDelegate.AddLambda(Callback);

		// Try to join a lobby using the ID which will call the callback when done
		LobbySubsystem->JoinLobbyByUserID(UserID);
	}
	else
	{
		FJoinLobbyResult Result;
		Result.LobbyResultCode = ELobbyResultCode::InvalidUserID;
		OnFailure.Broadcast(Result);
	}
}

// ------------------------------ End Join Lobby by User ID ------------------------------
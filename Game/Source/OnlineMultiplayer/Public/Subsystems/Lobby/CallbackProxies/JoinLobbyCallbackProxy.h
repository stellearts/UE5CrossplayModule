// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "Subsystems/Lobby/LobbySubsystem.h"
#include "JoinLobbyCallbackProxy.generated.h"



USTRUCT(BlueprintType)
struct FJoinLobbyResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FLobbyDetails LobbyDetails;

	UPROPERTY(BlueprintReadWrite)
	EJoinLobbyResultCode LobbyResultCode;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FProxyJoinLobbyCompleteDelegate, const FJoinLobbyResult&, Result);



UCLASS(MinimalAPI)
class UJoinLobbyByLobbyIDCallbackProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_UCLASS_BODY()

	// Called when the lobby was joined successfully
	UPROPERTY(BlueprintAssignable)
	FProxyJoinLobbyCompleteDelegate OnSuccess;

	// Called when there was an error joining the lobby
	UPROPERTY(BlueprintAssignable)
	FProxyJoinLobbyCompleteDelegate OnFailure;
	
	// Join a lobby by Lobby-ID
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "Lobby|Functions")
	static UJoinLobbyByLobbyIDCallbackProxy* JoinLobbyByID(UObject* WorldContextObject, FString LobbyID);
	
	virtual void Activate() override;
	
private:
	FString LobbyID;
	UPROPERTY() UObject* WorldContextObject;
	FDelegateHandle JoinLobbyDelegateHandle;
};



UCLASS(MinimalAPI)
class UJoinLobbyByUserIDCallbackProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_UCLASS_BODY()

	// Called when the lobby was joined successfully
	UPROPERTY(BlueprintAssignable)
	FProxyJoinLobbyCompleteDelegate OnSuccess;

	// Called when there was an error joining the lobby
	UPROPERTY(BlueprintAssignable)
	FProxyJoinLobbyCompleteDelegate OnFailure;
	
	// Join a lobby by User-ID
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "Online|Lobby")
	static UJoinLobbyByUserIDCallbackProxy* JoinLobbyByUserID(UObject* WorldContextObject, FString UserID);
	
	virtual void Activate() override;
	
private:
	UPROPERTY() UObject* WorldContextObject;
	FString UserID;
	FDelegateHandle JoinLobbyDelegateHandle;
};

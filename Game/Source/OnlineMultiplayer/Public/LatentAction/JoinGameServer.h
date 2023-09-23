// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "JoinGameServer.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FProxyJoinGameServerCompleteDelegate);



UCLASS(MinimalAPI)
class UJoinGameServer : public UOnlineBlueprintCallProxyBase
{
	GENERATED_UCLASS_BODY()
	
	UPROPERTY(BlueprintAssignable)
	FProxyJoinGameServerCompleteDelegate OnSuccess;
	
	UPROPERTY(BlueprintAssignable)
	FProxyJoinGameServerCompleteDelegate OnFailure;
	
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"), Category = "Server")
	static UJoinGameServer* JoinGameServer(FString ServerAddress, APlayerController* PlayerController);
	
	virtual void Activate() override;
	void ServerJoined(UWorld* NewWorld);
	
	
private:
	UPROPERTY() UWorld* World;
	UPROPERTY() FString ServerAddress;
	UPROPERTY() APlayerController* PlayerController;

	FDelegateHandle JoinServerCompleteDelegateHandle;
};

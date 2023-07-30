// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "LobbySubsystem.h"
#include "Engine/Engine.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "CreateLobbyCallbackProxy.generated.h"



USTRUCT(BlueprintType)
struct FCreateLobbyResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FLobbyDetails LobbyDetails;

	UPROPERTY(BlueprintReadWrite)
	ELobbyResultCode LobbyResultCode;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCreateLobbyCompleteDelegate, const FCreateLobbyResult&, Result);



UCLASS(MinimalAPI)
class UCreateLobbyCallbackProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_UCLASS_BODY()

	// Called when the lobby was created successfully
	UPROPERTY(BlueprintAssignable)
	FCreateLobbyCompleteDelegate OnSuccess;

	// Called when there was an error creating the lobby
	UPROPERTY(BlueprintAssignable)
	FCreateLobbyCompleteDelegate OnFailure;
	
	// Create a lobby
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "Online|Lobby")
	static UCreateLobbyCallbackProxy* CreateLobby(UObject* WorldContextObject);
	
	virtual void Activate() override;
	
private:
	UPROPERTY() UObject* WorldContextObject;
};

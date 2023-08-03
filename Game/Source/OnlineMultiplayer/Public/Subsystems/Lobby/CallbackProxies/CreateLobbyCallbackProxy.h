// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "Subsystems/Lobby/LobbySubsystem.h"
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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FProxyCreateLobbyCompleteDelegate, const FCreateLobbyResult&, Result);



UCLASS(MinimalAPI)
class UCreateLobbyCallbackProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_UCLASS_BODY()

	// Called when the lobby was created successfully
	UPROPERTY(BlueprintAssignable)
	FProxyCreateLobbyCompleteDelegate OnSuccess;

	// Called when there was an error creating the lobby
	UPROPERTY(BlueprintAssignable)
	FProxyCreateLobbyCompleteDelegate OnFailure;
	
	// Create a lobby
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "Online|Lobby")
	static UCreateLobbyCallbackProxy* CreateLobby(UObject* WorldContextObject, const int32 MaxMembers);
	
	virtual void Activate() override;
	
private:
	UPROPERTY() UObject* WorldContextObject;
	UPROPERTY() int32 MaxMembers;
};

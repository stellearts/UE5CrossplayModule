// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "Subsystems/Lobby/LobbySubsystem.h"
#include "LeaveLobbyCallbackProxy.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FProxyLeaveLobbyCompleteDelegate, const ELobbyResultCode, ResultCode);



UCLASS(MinimalAPI)
class ULeaveLobbyCallbackProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_UCLASS_BODY()

	// Called when the lobby was left successfully
	UPROPERTY(BlueprintAssignable)
	FProxyLeaveLobbyCompleteDelegate OnSuccess;

	// Called when there was an error leaving the lobby
	UPROPERTY(BlueprintAssignable)
	FProxyLeaveLobbyCompleteDelegate OnFailure;
	
	// Leave the current lobby
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "Lobby|Functions")
	static ULeaveLobbyCallbackProxy* LeaveLobby(UObject* WorldContextObject, const int32 MaxMembers);
	
	virtual void Activate() override;
	
private:
	UPROPERTY() UObject* WorldContextObject;
};

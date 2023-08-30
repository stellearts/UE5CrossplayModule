// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "StartListenServer.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FProxyStartListenServerCompleteDelegate);



UCLASS(MinimalAPI)
class UStartListenServer : public UOnlineBlueprintCallProxyBase
{
	GENERATED_UCLASS_BODY()
	
	UPROPERTY(BlueprintAssignable)
	FProxyStartListenServerCompleteDelegate OnSuccess;
	
	UPROPERTY(BlueprintAssignable)
	FProxyStartListenServerCompleteDelegate OnFailure;

	/**
	 * Will start an active listen server.
	 * 
	 * If there is an active lobby or session, then all members will be requested to join, and this function will wait until all of them have joined before completing.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "Server")
	static UStartListenServer* StartListenServer(UObject* WorldContextObject);
	
	virtual void Activate() override;
	void OnHttpRequestCompleted(TSharedPtr<class IHttpRequest, ESPMode::ThreadSafe> Request, TSharedPtr<class IHttpResponse, ESPMode::ThreadSafe> Response, bool bWasSuccessful);
	
	
private:
	UPROPERTY() UWorld* World;
	FDelegateHandle StartServerCompleteDelegateHandle;
};

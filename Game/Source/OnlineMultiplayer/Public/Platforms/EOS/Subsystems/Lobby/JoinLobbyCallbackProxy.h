// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "JoinLobbyCallbackProxy.generated.h"



USTRUCT(BlueprintType)
struct FJoinLobbyResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	TArray<class UEosUser*> MemberList;

	UPROPERTY(BlueprintReadWrite)
	FText ErrorMessage;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJoinLobbyCompleteDelegate, const FJoinLobbyResult&, Result);



UCLASS(MinimalAPI)
class UJoinLobbyCallbackProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_UCLASS_BODY()

	// Called when the lobby was joined successfully
	UPROPERTY(BlueprintAssignable)
	FJoinLobbyCompleteDelegate OnSuccess;

	// Called when there was an error joining the lobby
	UPROPERTY(BlueprintAssignable)
	FJoinLobbyCompleteDelegate OnFailure;
	
	// Join a lobby by ID
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "Online|Lobby")
	static UJoinLobbyCallbackProxy* JoinLobbyByID(UObject* WorldContextObject, FString LobbyID);
	
	virtual void Activate() override;
	
private:
	FString LobbyID;
	UPROPERTY() UObject* WorldContextObject;
};

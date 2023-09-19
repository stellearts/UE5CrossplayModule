// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "eos_sdk.h"
#include "Types/UserTypes.h"
#include "ConnectSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogConnectSubsystem, Log, All);
inline DEFINE_LOG_CATEGORY(LogConnectSubsystem);

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnConnectLoginCompleteDelegate, const bool bSuccess, UOnlineUser* LocalUser);



/**
 * Subsystem for authorizing users for both Auth and Connect interfaces.
 *
 * Connect is used for multiplayer and matchmaking.
 *
 * Auth is used for user accounts and friends.
 */
UCLASS(BlueprintType)
class ONLINEMULTIPLAYER_API UConnectSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

public:
	UFUNCTION(BlueprintCallable, Category = "Account")
	void Login();
	UFUNCTION(BlueprintCallable, Category = "Account")
	void Logout();

private:
	static void EOS_CALL OnLoginComplete(const EOS_Connect_LoginCallbackInfo* Data);
	void OnLogoutComplete();

public:
	void GetOnlineUserDetails(TArray<FString>& ProductUserIDList, const TFunction<void(TArray<UOnlineUser*>)> &Callback);

private:
	void CreateNewUser();
	void CheckAccounts();

	class FEosManager* EosManager;
	class FSteamManager* SteamManager;
	
	EOS_HConnect ConnectHandle;
	EOS_ProductUserId EosProductUserId;
	EOS_ContinuanceToken EosContinuanceToken;

	UPROPERTY()
	class ULocalUserSubsystem* LocalUserSubsystem;
	UPROPERTY()
	class UOnlineUserSubsystem* OnlineUserSubsystem;

public:
	FOnConnectLoginCompleteDelegate OnConnectLoginCompleteDelegate;
};

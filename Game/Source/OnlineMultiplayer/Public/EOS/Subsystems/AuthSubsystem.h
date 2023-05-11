// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "eos_sdk.h"
#include "AuthSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAuthSubsystem, Log, All);
inline DEFINE_LOG_CATEGORY(LogAuthSubsystem);



/**
 * Subsystem for authorizing users for both Auth and Connect interfaces.
 *
 * Connect is used for multiplayer and matchmaking.
 *
 * Auth is used for user accounts and friends.
 */
UCLASS(BlueprintType)
class ONLINEMULTIPLAYER_API UAuthSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

public:
	UFUNCTION(BlueprintCallable, Category = "Online|Login|Auth")
	void LoginAuth();
	UFUNCTION(BlueprintCallable, Category = "Online|Login|Auth")
	void LogoutAuth();
	
	UFUNCTION(BlueprintCallable, Category = "Online|Login|Connect")
	void LoginConnect();
	UFUNCTION(BlueprintCallable, Category = "Online|Login|Connect")
	void LogoutConnect();

private:
	static void EOS_CALL OnLoginAuthComplete(const EOS_Auth_LoginCallbackInfo* Data);
	void OnLogoutAuthComplete();
	static void EOS_CALL OnLoginConnectComplete(const EOS_Connect_LoginCallbackInfo* Data);
	void OnLogoutConnectComplete();
	
	void LinkUserAuth();
	void CreateNewUserConnect();
	void CheckAccounts();

	class FEosManager* EosManager;
	TSharedPtr<class FLocalUserState> LocalUserState;
	EOS_HAuth AuthHandle;
	EOS_HConnect ConnectHandle;
	EOS_ProductUserId EosProductUserId;
	EOS_ContinuanceToken EosContinuanceToken;
};

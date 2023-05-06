// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "eos_sdk.h"
#include "AuthSubsystemEOS.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAuthSubsystemEOS, Log, All);
inline DEFINE_LOG_CATEGORY(LogAuthSubsystemEOS);



/**
 * Authorization subsystem for EOS.
 * For both Auth and Connect interfaces.
 */
UCLASS(BlueprintType)
class ONLINEMULTIPLAYER_API UAuthSubsystemEOS : public UGameInstanceSubsystem
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
	static void EOS_CALL OnLoginConnectComplete(const EOS_Connect_LoginCallbackInfo* Data);
	void OnLogoutConnectComplete();

	static void EOS_CALL OnLoginAuthComplete(const EOS_Auth_LoginCallbackInfo* Data);
	void OnLogoutAuthComplete();

	class FEosManager* EosManager;
	EOS_HAuth AuthHandle;
	EOS_HConnect ConnectHandle;
	EOS_ProductUserId EosProductUserId;
	EOS_ContinuanceToken EosContinuanceToken;
};

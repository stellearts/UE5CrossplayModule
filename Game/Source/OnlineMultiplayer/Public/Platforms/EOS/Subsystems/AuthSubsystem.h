// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "eos_sdk.h"
#include "OnlineMultiplayer_CommonTypes.h"
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
	UFUNCTION(BlueprintCallable, Category = "Account")
	void Login();
	UFUNCTION(BlueprintCallable, Category = "Account")
	void Logout();

private:
	static void EOS_CALL OnLoginComplete(const EOS_Auth_LoginCallbackInfo* Data);
	void OnLogoutComplete();

	void LinkUserAuth();

	class FEosManager* EosManager;
	class FSteamManager* SteamManager;
	
	EOS_HAuth AuthHandle;
	EOS_ProductUserId EosProductUserId;
	EOS_ContinuanceToken EosContinuanceToken;
	
	UPROPERTY()
	class ULocalUserSubsystem* LocalUserSubsystem;
};

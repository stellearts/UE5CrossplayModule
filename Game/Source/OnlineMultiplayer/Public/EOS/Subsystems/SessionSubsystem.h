// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "eos_sdk.h"
#include "SessionSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSessionSubsystem, Log, All);
inline DEFINE_LOG_CATEGORY(LogSessionSubsystem);



/**
 * Subsystem for managing sessions.
 */
UCLASS(BlueprintType)
class ONLINEMULTIPLAYER_API USessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

public:
	UFUNCTION(BlueprintCallable, Category = "Online|Session")
	void CreateSession();

private:
	static void OnCreateSessionComplete(const EOS_Sessions_UpdateSessionCallbackInfo* Data);
	
	class FEosManager* EosManager;
	TSharedPtr<class FLocalUserState> LocalUserState;
	EOS_HSessions SessionsHandle;
};

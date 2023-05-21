// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "eos_sdk.h"

DECLARE_LOG_CATEGORY_EXTERN(LogEOSSubsystem, Log, All);
inline DEFINE_LOG_CATEGORY(LogEOSSubsystem);



/**
 * Responsible for initializing the SDK.
 * 
 * Other classes can get the platform-handle from this class.
 */
class ONLINEMULTIPLAYER_API FEosManager final : public FTickableGameObject
{
	// Private constructor to prevent direct instantiation
	FEosManager() = default;
	virtual ~FEosManager() override;
	
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
	
public:
	static FEosManager& Get();
	void Initialize();

private:
	void InitializeSdk();
	void InitializePlatform();
	EOS_EResult CreateIntegratedPlatform(EOS_Platform_Options& PlatformOptions);
	void FreeIntegratedPlatform(EOS_Platform_Options& PlatformOptions);

	
	EOS_HPlatform PlatformHandle;
	
public:
	FORCEINLINE EOS_HPlatform GetPlatformHandle() const { return PlatformHandle; }
};

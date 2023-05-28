// Copyright © 2023 Melvin Brink

#pragma once

#include <string>
#include "CoreMinimal.h"
#include "Platforms/EOS/UserTypes.h"
#include "eos_sdk.h"
#include "OnlineUserSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogOnlineUserSubsystem, Log, All);
inline DEFINE_LOG_CATEGORY(LogOnlineUserSubsystem);



/**
 * Subsystem for managing the user.
 */
UCLASS(BlueprintType)
class ONLINEMULTIPLAYER_API UOnlineUserSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UOnlineUserSubsystem();

private:
	class FSteamManager& SteamManager;
	class FEosManager& EosManager;
	UPROPERTY() class USteamUserSubsystem* SteamUserSubsystem;

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

private:
	FUserState UserState;

public:
	// UserState Getters
	FORCEINLINE EOS_ProductUserId GetProductUserID() const { return UserState.ProductUserID; }
	FORCEINLINE EOS_EpicAccountId GetEpicAccountID() const { return UserState.EpicAccountID; }
	FORCEINLINE EOS_EExternalAccountType GetPlatform() const { return UserState.Platform; }
	FORCEINLINE std::string GetPlatformID() const { return UserState.PlatformID; }
	FORCEINLINE std::string GetDisplayName() const { return UserState.DisplayName; }
	FORCEINLINE std::string GetAvatarURL() const { return UserState.AvatarURL; }

	// UserState Setters
	void SetProductUserId(const EOS_ProductUserId ProductUserId) { UserState.ProductUserID = ProductUserId; }
	void SetEpicAccountId(const EOS_EpicAccountId EpicAccountId) { UserState.EpicAccountID = EpicAccountId; }
	void SetPlatform(const EOS_EExternalAccountType PlatformType) { UserState.Platform = PlatformType; }
	void SetPlatformID(const std::string PlatformID) { UserState.PlatformID = PlatformID; }
	void SetDisplayName(const std::string DisplayName) { UserState.DisplayName = DisplayName; }
	void SetAvatarURL(const std::string AvatarURL) { UserState.AvatarURL = AvatarURL; }


	
	// ------------------------------- Online User Utilities -------------------------------
	
	
};

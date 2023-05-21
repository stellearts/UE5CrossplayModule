// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "SteamFriendsSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSteamFriendsSubsystem, Log, All);
inline DEFINE_LOG_CATEGORY(LogSteamFriendsSubsystem);



/**
 * Subsystem for managing Steam friends.
 */
UCLASS(NotBlueprintable)
class ONLINEMULTIPLAYER_API USteamFriendsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
};

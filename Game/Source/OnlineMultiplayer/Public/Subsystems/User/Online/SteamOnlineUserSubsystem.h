// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include <string>

#pragma warning(push)
#pragma warning(disable: 4996)
#pragma warning(disable: 4265)
#include <vector>

#include "steam_api.h"
#include "steam_api_common.h"
#include "steamnetworkingtypes.h"
#pragma warning(pop)

#include "SteamOnlineUserSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSteamOnlineUserSubsystem, Log, All);
inline DEFINE_LOG_CATEGORY(LogSteamOnlineUserSubsystem);



/**
 * Subsystem for managing Steam users.
 */
UCLASS(NotBlueprintable)
class ONLINEMULTIPLAYER_API USteamOnlineUserSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

public:
	void FetchAvatar(const uint64 UserID, const TFunction<void(UTexture2D*)> &Callback);

private:
	UTexture2D* ProcessAvatar(const int& ImageData);
	STEAM_CALLBACK(USteamOnlineUserSubsystem, OnPersonaStateChange, PersonaStateChange_t);
	STEAM_CALLBACK(USteamOnlineUserSubsystem, OnAvatarImageLoaded, AvatarImageLoaded_t);

	TMap<uint64, const TFunction<void(UTexture2D*)>> FetchAvatarCallbacks;
};

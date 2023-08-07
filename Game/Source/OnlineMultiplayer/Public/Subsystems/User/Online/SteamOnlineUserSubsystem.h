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

enum EFriendStateChangedType
{
	
};

// struct FFriendStateChangedData
// {
// 	std::string UserID;
// 	EFriendStateChangedType Avatar;
// };
//
// DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFriendStateChanged, const FFriendStateChangedData, Data);



/**
 * Subsystem for managing Steam friends.
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
	UTexture2D* ProcessAvatar(const CSteamID& SteamUserID);
	UTexture2D* USteamOnlineUserSubsystem::BufferToTexture2D(std::vector<uint8>& Buffer, uint32 Width, uint32 Height);
	STEAM_CALLBACK(USteamOnlineUserSubsystem, OnPersonaStateChange, PersonaStateChange_t);

	TMap<uint64, const TFunction<void(UTexture2D*)>> FetchAvatarCallbacks;
};

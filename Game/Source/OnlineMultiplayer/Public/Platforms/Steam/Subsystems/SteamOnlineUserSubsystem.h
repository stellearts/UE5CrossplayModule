// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include <string>

#pragma warning(push)
#pragma warning(disable: 4996)
#pragma warning(disable: 4265)
#include "steam_api.h"
#include "steam_api_common.h"
#include "steamnetworkingtypes.h"
#pragma warning(pop)

#include "SteamOnlineUserSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSteamOnlineUserSubsystem, Log, All);
inline DEFINE_LOG_CATEGORY(LogSteamOnlineUserSubsystem);

#define CHECK_STEAM if(!SteamAPI_Init()){ UE_LOG(LogSteamOnlineUserSubsystem, Error, TEXT("Steam SDK is not initialized.")); return; }

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
	void FetchAvatar(const uint64& UserIDString, const TFunction<void>& Callback);

private:
	void ProcessAvatar(const CSteamID& UserID);
	STEAM_CALLBACK(USteamOnlineUserSubsystem, OnPersonaStateChange, PersonaStateChange_t);
};

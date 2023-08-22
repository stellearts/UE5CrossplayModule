// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "Types/UserTypes.h"
#include "OnlineUserSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogOnlineUserSubsystem, Log, All);
inline DEFINE_LOG_CATEGORY(LogOnlineUserSubsystem);



UENUM(BlueprintType)
enum class EGetOnlineUserResultCode : uint8
{
	Success UMETA(DisplayName = "Success."),
	Failed UMETA(DisplayName = "Failed to get the details of one or more user's.")
};

USTRUCT(BlueprintType)
struct FGetOnlineUserResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	UOnlineUser* OnlineUser;

	UPROPERTY(BlueprintReadOnly)
	EGetOnlineUserResultCode ResultCode;
};

USTRUCT(BlueprintType)
struct FGetOnlineUsersResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TArray<UOnlineUser*> OnlineUsers;

	UPROPERTY(BlueprintReadOnly)
	EGetOnlineUserResultCode ResultCode;
};



/**
 * Subsystem for managing user's from the friend, lobby or session lists.
 *
 * Provides helper functions for getting certain user data.
 */
UCLASS(BlueprintType)
class ONLINEMULTIPLAYER_API UOnlineUserSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UOnlineUserSubsystem();

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

private:
	class FSteamManager* SteamManager;
	class FEosManager* EosManager;
	UPROPERTY() class USteamLocalUserSubsystem* SteamLocalUserSubsystem;
	UPROPERTY() class USteamOnlineUserSubsystem* SteamOnlineUserSubsystem;
	
	UPROPERTY() TMap<FString, UOnlineUser*> CachedOnlineUsers;

public:
	void GetOnlineUser(const FString& ProductUserID, const TFunction<void(FGetOnlineUserResult)> &Callback);
	void GetOnlineUsers(TArray<FString>& ProductUserIDs,const TFunction<void(FGetOnlineUsersResult)> &Callback);
	
	void LoadUserAvatar(const UOnlineUser* OnlineUser, const TFunction<void>& Callback);
};

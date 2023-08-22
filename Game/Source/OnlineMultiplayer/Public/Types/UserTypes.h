// Copyright © 2023 Melvin Brink

#pragma once

#include "eos_common.h"
#include "UserTypes.generated.h"



// // These are the external account types that EOS supports
// struct FExternalAccount {
// 	FString ProductUserID;
// 	FString DisplayName;
// 	FString AccountID;
// 	EOS_EExternalAccountType AccountType;
// 	int64_t LastLoginTime;
// };
// typedef TMap<EOS_EExternalAccountType, FExternalAccount> FExternalAccountsMap;

UENUM(BlueprintType)
enum class EPlatform : uint8
{
	Steam UMETA(DisplayName = "Steam"),
	Psn UMETA(DisplayName = "Playstation Network"),
	Xbox UMETA(DisplayName = "Xbox"),
	Epic UMETA(DisplayName = "Epic Games"),
};


/**
 * The platform user is a user on a specific platform.
 * For example; Steam or Playstation user's.
 */
USTRUCT(BlueprintType)
struct FPlatformUser
{
	GENERATED_BODY()

	FString UserID;
	FString Username;
	UPROPERTY() UTexture2D* Avatar;
	EPlatform Platform;
	int64_t LastLoginTime;
};


/**
 * The online user is a user that joins a lobby / session.
 * Has the extra properties required when interacting with them.
 */
UCLASS(BlueprintType)
class UOnlineUser : public UObject
{
	GENERATED_BODY()

protected:
	UPROPERTY()
	FString ProductUserID;

	UPROPERTY()
	FString EpicAccountID;

	UPROPERTY()
	EPlatform Platform;

	UPROPERTY()
	FPlatformUser PlatformUser;

	UPROPERTY()
	TMap<EPlatform, FPlatformUser> ExternalPlatformUsers;

public:
	// Getters
	UFUNCTION(BlueprintPure, Category = "User|Details") FORCEINLINE FString& GetProductUserID() { return ProductUserID; }
	UFUNCTION(BlueprintPure, Category = "User|Details") FORCEINLINE FString& GetEpicAccountID() { return EpicAccountID; }
	UFUNCTION(BlueprintPure, Category = "User|Details") FORCEINLINE FString GetUserID() const { return PlatformUser.UserID; }
	UFUNCTION(BlueprintPure, Category = "User|Details") FORCEINLINE FString GetUsername() const { return PlatformUser.Username; }
	UFUNCTION(BlueprintPure, Category = "User|Details") FORCEINLINE UTexture2D* GetAvatar() const { return PlatformUser.Avatar; }
	UFUNCTION(BlueprintPure, Category = "User|Details") FORCEINLINE EPlatform GetPlatform() const { return Platform; }

	// Setters
	FORCEINLINE void SetProductUserID(const FString &InProductUserID) { ProductUserID = InProductUserID; }
	FORCEINLINE void SetEpicAccountID(const FString &InEpicAccountID) { EpicAccountID = InEpicAccountID; }
	FORCEINLINE void SetUserID(const FString &InUserID) { PlatformUser.UserID = InUserID; }
	FORCEINLINE void SetUserID(const uint64 &InUserID) { PlatformUser.UserID = FString::Printf(TEXT("%llu"), InUserID); }
	FORCEINLINE void SetUsername(const FString &InUsername) { PlatformUser.Username = InUsername; }
	FORCEINLINE void SetAvatar(UTexture2D *InAvatar) { PlatformUser.Avatar = InAvatar; }
	FORCEINLINE void SetPlatform(const EPlatform InPlatform) { Platform = InPlatform; }

	FORCEINLINE void SetExternalPlatformUsers(const TMap<EPlatform, FPlatformUser>& InExternalPlatformUsers) { ExternalPlatformUsers = InExternalPlatformUsers; }
	FORCEINLINE void AddExternalPlatformUser(const EPlatform InPlatform, FPlatformUser InPlatformUser) { ExternalPlatformUsers.Add(InPlatform, PlatformUser); }
	FORCEINLINE void SetPlatformUser(const FPlatformUser &InPlatformUser) { PlatformUser = InPlatformUser; }
	bool SetPlatformUser(const EPlatform InPlatform)
	{
		const FPlatformUser* PlatformUserPtr = ExternalPlatformUsers.Find(InPlatform);
		if(!PlatformUserPtr) return false;

		PlatformUser = *PlatformUserPtr;
		return true;
	}
};


/**
 * The local user is an UOnlineUser with extra properties and helper methods.
 * This is the user on the client, the local player.
 */
UCLASS(BlueprintType)
class ULocalUser : public UOnlineUser
{
	GENERATED_BODY()
	
	EOS_ContinuanceToken ContinuanceToken;
	// TODO: Add properties like SteamSessionTickets here.

public:
	// Getters
	FORCEINLINE EOS_ContinuanceToken GetContinuanceToken() const { return ContinuanceToken; }

	// Setters
	FORCEINLINE void SetContinuanceToken(const EOS_ContinuanceToken& InContinuanceToken) { ContinuanceToken = InContinuanceToken; }

	// Helper functions to make the code more readable
	FORCEINLINE bool IsAuthLoggedIn() const { return !EpicAccountID.IsEmpty(); }
	FORCEINLINE bool IsConnectLoggedIn() const { return !ProductUserID.IsEmpty(); }
};
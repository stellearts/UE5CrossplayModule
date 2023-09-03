// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "Types/UserTypes.h"
#include "LobbyTypes.generated.h"



/*
 * Attributes
 */

/*
 * Used to check of which type the attribute is.
 */
UENUM(BlueprintType)
enum class ELobbyAttributeType : uint8
{
	Bool, // bool
	String, // FString
	Int64, // int64_t
	Double, // double
};

/*
 * Single lobby attribute.
 */
USTRUCT(BlueprintType)
struct FLobbyAttribute
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString Key;

	UPROPERTY(BlueprintReadWrite)
	ELobbyAttributeType Type;

	UPROPERTY(BlueprintReadWrite)
	FString StringValue;

	UPROPERTY(BlueprintReadWrite)
	bool BoolValue;

	UPROPERTY(BlueprintReadWrite)
	int64 IntValue;

	UPROPERTY(BlueprintReadWrite)
	double DoubleValue;
};



/*
 * Settings
 */

USTRUCT(BlueprintType)
struct FLobbySettings
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 MaxMembers = 4;
};



/*
 * The Lobby
 */

/*
 * Stores all information about a lobby.
 */
USTRUCT(BlueprintType)
struct FLobby
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<FString, FLobbyAttribute> Attributes;

	UPROPERTY()
	FLobbySettings Settings;

	UPROPERTY(BlueprintReadOnly)
	FString ID = FString("");

	UPROPERTY(BlueprintReadOnly)
	FString OwnerID = FString(""); // TODO: Set when owner leaves lobby.

	UPROPERTY(BlueprintReadOnly)
	TMap<FString, UOnlineUser*> MemberList;


	
	FORCEINLINE void AddMember(UOnlineUser* OnlineUser) { MemberList.Add(OnlineUser->GetProductUserID(), OnlineUser); }
	FORCEINLINE void RemoveMember(const FString& ProductUserID) { MemberList.Remove(ProductUserID); }
	FORCEINLINE UOnlineUser** GetMember(const FString& ProductUserID) { return MemberList.Find(ProductUserID); }
	
	TArray<UOnlineUser*> GetMemberList() const
	{
		TArray<UOnlineUser*> OutArr;
		MemberList.GenerateValueArray(OutArr);
		return OutArr;
	}

	// Sets everything to default values
	void Reset()
	{
		ID = "";
		OwnerID = "";
		MemberList.Empty();
		Attributes.Empty();
		Settings = FLobbySettings();
	}
};



/*
 * Result Codes
 */

UENUM(BlueprintType)
enum class ECreateLobbyResultCode : uint8
{
	Success UMETA(DisplayName = "Success."),
	Failure UMETA(DisplayName = "Failed to create the lobby."),
	PresenceLobbyExists UMETA(DisplayName = "A presence-lobby already exists."),
	InLobby UMETA(DisplayName = "Already in a lobby."),
	EosFailure UMETA(DisplayName = "Some Epic Online Services SDK functionality failed."),
	Unknown UMETA(DisplayName = "Unkown error occurred."),
};

UENUM(BlueprintType)
enum class EJoinLobbyResultCode : uint8
{
	Success UMETA(DisplayName = "Success."),
	Failure UMETA(DisplayName = "Failed to join the lobby."),
	PresenceLobbyExists UMETA(DisplayName = "A presence-lobby already exists."),
	NotFound UMETA(DisplayName = "No lobby was found."),
	InvalidLobbyID UMETA(DisplayName = "Invalid lobby ID."),
	InvalidUserID UMETA(DisplayName = "Invalid user ID."),
	InLobby UMETA(DisplayName = "Already in a lobby."),
	EosFailure UMETA(DisplayName = "Some Epic Online Services SDK functionality failed."),
	Unknown UMETA(DisplayName = "Unkown error occurred."),
};

UENUM(BlueprintType)
enum class ELeaveLobbyResultCode : uint8
{
	Success UMETA(DisplayName = "Success."),
	NotInLobby UMETA(DisplayName = "Not in a lobby."),
	Failure UMETA(DisplayName = "Failed to leave the lobby."),
	EosFailure UMETA(DisplayName = "Some Epic Online Services SDK functionality failed."),
	Unknown UMETA(DisplayName = "Unkown error occurred."),
};
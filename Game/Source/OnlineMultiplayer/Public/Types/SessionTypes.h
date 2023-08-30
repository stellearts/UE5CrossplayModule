// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "Types/UserTypes.h"
#include "SessionTypes.generated.h"



/*
 * Attributes
 */

/*
 * Used to check of which type the attribute is.
 */
UENUM(BlueprintType)
enum class ESessionAttributeType : uint8
{
	Bool, // bool
	String, // FString
	Int64, // int64_t
	Double, // double
};

/*
 * Single session attribute.
 */
USTRUCT(BlueprintType)
struct FSessionAttribute
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString Key;

	UPROPERTY(BlueprintReadWrite)
	ESessionAttributeType Type;

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
struct FSessionSettings
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString Name;

	UPROPERTY(BlueprintReadWrite)
	int32 MaxMembers = 4;
};



/*
 * The Session
 */


/*
 * Stores all information about a session.
 */
USTRUCT(BlueprintType)
struct FSession
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString Name;

	UPROPERTY()
	TMap<FString, FSessionAttribute> Attributes;

	UPROPERTY()
	FSessionSettings Settings;

	UPROPERTY(BlueprintReadOnly)
	FString ID = FString("");

	UPROPERTY(BlueprintReadOnly)
	FString OwnerID = FString(""); // TODO: Set when owner leaves session.

	UPROPERTY(BlueprintReadOnly)
	TMap<FString, UOnlineUser*> MemberList;


	
	FORCEINLINE void AddMember(UOnlineUser* OnlineUser) { MemberList.Add(OnlineUser->GetProductUserID(), OnlineUser); }
	FORCEINLINE void RemoveMember(const FString& ProductUserID) { MemberList.Remove(ProductUserID); }

	// Sets everything to default values
	void Reset()
	{
		ID = "";
		OwnerID = "";
		MemberList.Empty();
		Attributes.Empty();
		Settings = FSessionSettings();
	}
};



/*
* Result Codes
 */

UENUM(BlueprintType)
enum class ECreateSessionResultCode : uint8
{
	Success UMETA(DisplayName = "Success."),
	Failure UMETA(DisplayName = "Failed to create the session."),
	PresenceLobbyExists UMETA(DisplayName = "A presence-session already exists."),
	InSession UMETA(DisplayName = "Already in a session."),
	EosFailure UMETA(DisplayName = "Some Epic Online Services SDK functionality failed."),
	Unknown UMETA(DisplayName = "Unkown error occurred."),
};
// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "eos_sdk.h"
#include "UserTypes.h"
#include "LobbySubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogLobbySubsystem, Log, All);
inline DEFINE_LOG_CATEGORY(LogLobbySubsystem);



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

/*
 * Stores all lobby attributes.
 */
USTRUCT(BlueprintType)
struct FLobbyAttributes
{
	GENERATED_BODY()

	UPROPERTY()
	bool GameStarted = false;

	UPROPERTY()
	FString SteamLobbyID;
};

/*
 * Stores all the necessary lobby information.
 */
USTRUCT(BlueprintType)
struct FLobbyDetails
{
	GENERATED_BODY()

	UPROPERTY()
	FLobbyAttributes LobbyAttributes;

	UPROPERTY(BlueprintReadOnly)
	FString LobbyID = FString("");

	UPROPERTY(BlueprintReadOnly)
	FString LobbyOwnerID = FString("");; // TODO: Set when owner leaves lobby.

	UPROPERTY(BlueprintReadOnly)
	TMap<FString, UOnlineUser*> MemberList;

	UPROPERTY(BlueprintReadOnly)
	int32 MaxMembers = 4;

	FORCEINLINE void AddMember(UOnlineUser* OnlineUser) { MemberList.Add(OnlineUser->GetProductUserID(), OnlineUser); }
	FORCEINLINE void RemoveMember(const FString& ProductUserID) { MemberList.Remove(ProductUserID); }

	// Sets everything to default values
	void Reset()
	{
		// TODO: Check if MemberList needs to clear all pointers, or if empty is enough?
		LobbyID = "";
		LobbyOwnerID = "";
		MemberList.Empty();
		MaxMembers = 4;
	}
};


DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCreateLobbyCompleteDelegate, const ECreateLobbyResultCode ResultCode, const FLobbyDetails& LobbyDetails);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnJoinLobbyCompleteDelegate, const EJoinLobbyResultCode ResultCode, const FLobbyDetails& LobbyDetails);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLeaveLobbyCompleteDelegate, const ELeaveLobbyResultCode ResultCode);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyUserJoinedDelegate, const UOnlineUser* EosUser);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyUserLeftDelegate, const FString& ProductUserID);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyUserDisconnectedDelegate, const FString& ProductUserID);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyUserKickedDelegate, const FString& ProductUserID);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyUserPromotedDelegate, const FString& ProductUserID);

DECLARE_MULTICAST_DELEGATE(FOnGameStartDelegate);
DECLARE_MULTICAST_DELEGATE(FOnGameEndDelegate);


/**
 * Subsystem for managing game lobbies.
 *
 * A 'lobby' is a synonym for a 'party' in this case.
 */
UCLASS()
class ONLINEMULTIPLAYER_API ULobbySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
protected:
	ULobbySubsystem();
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

public:
	void CreateLobby(const int32 MaxMembers);
	void JoinLobbyByID(const FString& LobbyID);
	void JoinLobbyByUserID(const FString& UserID);
	void LeaveLobby();

private:
	void JoinLobbyByHandle(const EOS_HLobbyDetails& LobbyDetailsHandle);
	static void OnJoinLobbyComplete(const EOS_Lobby_JoinLobbyCallbackInfo* Data);

	EOS_HLobbyDetails GetLobbyDetailsHandle();

	template <typename T> void SetLobbyAttribute(const FString& Key, const T& Value);
	static void OnLobbyUpdate(const EOS_Lobby_LobbyUpdateReceivedCallbackInfo* Data);
	
	static void OnLobbyMemberStatusUpdate(const EOS_Lobby_LobbyMemberStatusReceivedCallbackInfo* Data);
	void OnLobbyUserJoined(const FString& TargetUserID);
	void OnLobbyUserLeft(const FString& TargetUserID);
	void OnLobbyUserDisconnected(const FString& TargetUserID);
	void OnLobbyUserKicked(const FString& TargetUserID);
	void OnLobbyUserPromoted(const FString& TargetUserID);

public:
	// Delegates
	FOnCreateLobbyCompleteDelegate OnCreateLobbyCompleteDelegate;
	FOnJoinLobbyCompleteDelegate OnJoinLobbyCompleteDelegate;
	FOnLeaveLobbyCompleteDelegate OnLeaveLobbyCompleteDelegate;
	
	FOnLobbyUserJoinedDelegate OnLobbyUserJoinedDelegate;
	FOnLobbyUserLeftDelegate OnLobbyUserLeftDelegate;
	FOnLobbyUserDisconnectedDelegate OnLobbyUserDisconnectedDelegate;
	FOnLobbyUserKickedDelegate OnLobbyUserKickedDelegate;
	FOnLobbyUserPromotedDelegate OnLobbyUserPromotedDelegate;

	FOnGameStartDelegate OnGameStartDelegate;
	FOnGameEndDelegate OnGameEndDelegate;

private:
	class FEosManager* EosManager;
	EOS_HLobby LobbyHandle;
	EOS_HLobbyDetails LobbyDetailsHandle;
	EOS_HLobbySearch LobbySearchByLobbyIDHandle;
	EOS_HLobbySearch LobbySearchByUserIDHandle;
	EOS_NotificationId OnLobbyUpdateNotification;
	EOS_NotificationId OnLobbyMemberStatusNotification;
	
	UPROPERTY() class UOnlineUserSubsystem* OnlineUserSubsystem;
	UPROPERTY() class ULocalUserSubsystem* LocalUserSubsystem;
	UPROPERTY() class UPlatformLobbySubsystemBase* LocalPlatformLobbySubsystem;
	
	UPROPERTY() FLobbyDetails LobbyDetails;
	void LoadLobbyDetails(const EOS_LobbyId LobbyID, TFunction<void(bool bSuccess)> OnCompleteCallback);

	TArray<FString> UsersToLoad; // Used to check if user's have left after loading their data.

public:
	FORCEINLINE const FLobbyDetails& GetLobbyDetails() const { return LobbyDetails; }
	FORCEINLINE bool InLobby() const { return !LobbyDetails.LobbyID.IsEmpty(); }




	
	/*
	 * Shadow lobby
	 */

	void OnCreateShadowLobbyComplete(const struct FShadowLobbyResult &ShadowLobbyResult);
	
	void JoinShadowLobby(const uint64 ShadowLobbyID);
	void OnJoinShadowLobbyComplete(const uint64 ShadowLobbyID);

	void AddShadowLobbyIDAttribute(const FString& ShadowLobbyID);
};
// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "eos_sdk.h"
#include "UserTypes.h"
#include "LobbySubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogLobbySubsystem, Log, All);
inline DEFINE_LOG_CATEGORY(LogLobbySubsystem);



UENUM(BlueprintType)
enum class ELobbyResultCode : uint8
{
	Success UMETA(DisplayName = "Success."),
	PresenceLobbyExists UMETA(DisplayName = "Presence lobby already exists."),
	CreateFailure UMETA(DisplayName = "Failed to create lobby."),
	JoinFailure UMETA(DisplayName = "Failed to join lobby."),
	SearchFailure UMETA(DisplayName = "Failed to find lobby."),
	LeaveFailure UMETA(DisplayName = "Failed to leave the lobby."),
	InvalidLobbyID UMETA(DisplayName = "Invalid Lobby ID."),
	InvalidUserID UMETA(DisplayName = "Invalid User ID."),
	InLobby UMETA(DisplayName = "Already in lobby."),
	EosFailure UMETA(DisplayName = "Some EOS functionality failed."),
	Unknown UMETA(DisplayName = "Unkown error occurred."),
};

/*
 * Struct for storing all the necessary lobby information.
 */
USTRUCT(BlueprintType)
struct FLobbyDetails
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString LobbyID = FString("");

	UPROPERTY(BlueprintReadOnly)
	FString LobbyOwnerID = FString("");;

	UPROPERTY(BlueprintReadOnly)
	TMap<FString, UOnlineUser*> MemberList;

	UPROPERTY(BlueprintReadOnly)
	int32 MaxMembers = 4;

	// Sets everything to default values
	void Reset()
	{
		LobbyID = "";
		LobbyOwnerID = "";
		MemberList.Empty();
		MaxMembers = 4;
	}
};


DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCreateLobbyCompleteDelegate, const ELobbyResultCode LobbyResultCode, const FLobbyDetails& LobbyDetails);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnJoinLobbyCompleteDelegate, const ELobbyResultCode LobbyResultCode, const FLobbyDetails& LobbyDetails);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLeaveLobbyCompleteDelegate, const ELobbyResultCode LobbyResultCode);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLobbyUserJoinedDelegate, UOnlineUser*, EosUser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyUserLeftDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyUserDisconnectedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyUserKickedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyUserPromotedDelegate);


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
	void JoinLobbyByHandle(const EOS_HLobbyDetails LobbyDetailsHandle);
	static void OnJoinLobbyComplete(const EOS_Lobby_JoinLobbyCallbackInfo* Data);
	
	static void OnLobbyUpdated(const EOS_Lobby_UpdateLobbyCallbackInfo* Data);
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
	
	UPROPERTY(BlueprintAssignable, Category = "Lobby|Events")
	FOnLobbyUserJoinedDelegate OnLobbyUserJoinedDelegate;
	UPROPERTY(BlueprintAssignable, Category = "Lobby|Events")
	FOnLobbyUserLeftDelegate OnLobbyUserLeftDelegate;
	UPROPERTY(BlueprintAssignable, Category = "Lobby|Events")
	FOnLobbyUserDisconnectedDelegate OnLobbyUserDisconnectedDelegate;
	UPROPERTY(BlueprintAssignable, Category = "Lobby|Events")
	FOnLobbyUserKickedDelegate OnLobbyUserKickedDelegate;
	UPROPERTY(BlueprintAssignable, Category = "Lobby|Events")
	FOnLobbyUserPromotedDelegate OnLobbyUserPromotedDelegate;

private:
	class FEosManager* EosManager;
	EOS_HLobby LobbyHandle;
	EOS_HLobbySearch LobbySearchByLobbyIDHandle;
	EOS_HLobbySearch LobbySearchByUserIDHandle;
	EOS_NotificationId OnLobbyMemberStatusUpdateID;

	UPROPERTY() class UPlatformLobbySubsystemBase* LocalPlatformLobbySubsystem;
	UPROPERTY() class UOnlineUserSubsystem* OnlineUserSubsystem;
	UPROPERTY() class ULocalUserSubsystem* LocalUserSubsystem;
	
	TArray<FString> UsersToLoad; // Used to check if user's have left after loading their data.
	UPROPERTY() FLobbyDetails LobbyDetails;
	void LoadLobbyDetails(const EOS_LobbyId LobbyID, TFunction<void(bool bSuccess, const FString& ErrorCode)> OnCompleteCallback);

public:
	// Lobby-Details Getters
	FORCEINLINE FString GetLobbyID() const { return LobbyDetails.LobbyID; }
	FORCEINLINE FString GetLobbyOwnerID() const { return LobbyDetails.LobbyOwnerID; }
	FORCEINLINE TMap<FString, UOnlineUser*> GetMemberMap() const {return LobbyDetails.MemberList;}
	TArray<UOnlineUser*> GetMemberList() const;
	UOnlineUser* GetMember(const EOS_ProductUserId ProductUserID);

	// Lobby-Details Setters
	FORCEINLINE void SetLobbyDetails(const FLobbyDetails& InLobbyDetails) { LobbyDetails = InLobbyDetails; }
	FORCEINLINE void SetLobbyID(const FString& InLobbyID) { LobbyDetails.LobbyID = InLobbyID; }
	FORCEINLINE void SetLobbyOwnerID(const FString& InLobbyOwnerID) { LobbyDetails.LobbyOwnerID = InLobbyOwnerID; }
	void StoreMember(UOnlineUser* OnlineUser);
	void StoreMembers(TArray<UOnlineUser*> &OnlineUserList);

	// Helpers
	FORCEINLINE bool InLobby() const { return !LobbyDetails.LobbyID.IsEmpty(); }




	
	/*
	 * Shadow lobby
	 */

	void OnCreateShadowLobbyComplete(const struct FShadowLobbyResult &ShadowLobbyResult);
	
	void JoinShadowLobby(const uint64 ShadowLobbyID);
	void OnJoinShadowLobbyComplete(const uint64 ShadowLobbyID);

	void AddShadowLobbyIDAttribute(const FString& ShadowLobbyID);
};
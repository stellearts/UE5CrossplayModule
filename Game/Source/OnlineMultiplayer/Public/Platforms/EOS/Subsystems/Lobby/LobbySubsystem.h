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
	Success UMETA(DisplayName = "No error."),
	PresenceLobbyExists UMETA(DisplayName = "Presence lobby already exists."),
	CreateFailure UMETA(DisplayName = "Failed to create lobby."),
	JoinFailure UMETA(DisplayName = "Failed to join lobby."),
	SearchFailure UMETA(DisplayName = "Failed to find lobby."),
	InLobby UMETA(DisplayName = "Already in lobby."),
	InvalidLobbyID UMETA(DisplayName = "Invalid Lobby ID."),
	InvalidUserID UMETA(DisplayName = "Invalid User ID."),
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
	FString LobbyID;

	UPROPERTY(BlueprintReadOnly)
	FString LobbyOwnerID;

	UPROPERTY(BlueprintReadOnly)
	TMap<FString, UEosUser*> MemberList;

	UPROPERTY(BlueprintReadOnly)
	int32 MaxMembers;
};


DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCreateLobbyCompleteDelegate, const ELobbyResultCode LobbyResultCode, const FLobbyDetails& LobbyDetails);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnJoinLobbyCompleteDelegate, const ELobbyResultCode LobbyResultCode, const FLobbyDetails& LobbyDetails);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLobbyUserJoinedDelegate, UEosUser*, EosUser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyUserLeftDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyUserDisconnectedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyUserKickedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyUserPromotedDelegate);


/**
 * Subsystem for managing game lobbies.
 *
 * A 'lobby' is a synonym for a 'party' in this case.
 */
UCLASS(BlueprintType)
class ONLINEMULTIPLAYER_API ULobbySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
protected:
	ULobbySubsystem();
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

public:
	
	UFUNCTION(BlueprintCallable, Category = "Online|Lobby")
	void CreateLobby(const int32 MaxMembers);
	void JoinLobbyByID(const FString& LobbyID);
	void JoinLobbyByUserID(const FString& UserID);

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
	// Create / Join
	FOnCreateLobbyCompleteDelegate OnCreateLobbyCompleteDelegate;
	FOnJoinLobbyCompleteDelegate OnJoinLobbyCompleteDelegate;
	
	// Lobby User
	UPROPERTY(BlueprintAssignable, Category = "Lobby|Delegates")
	FOnLobbyUserJoinedDelegate OnLobbyUserJoinedDelegate;
	UPROPERTY(BlueprintAssignable, Category = "Lobby|Delegates")
	FOnLobbyUserLeftDelegate OnLobbyUserLeftDelegate;
	UPROPERTY(BlueprintAssignable, Category = "Lobby|Delegates")
	FOnLobbyUserDisconnectedDelegate OnLobbyUserDisconnectedDelegate;
	UPROPERTY(BlueprintAssignable, Category = "Lobby|Delegates")
	FOnLobbyUserKickedDelegate OnLobbyUserKickedDelegate;
	UPROPERTY(BlueprintAssignable, Category = "Lobby|Delegates")
	FOnLobbyUserPromotedDelegate OnLobbyUserPromotedDelegate;

private:
	class FEosManager* EosManager;
	EOS_HLobby LobbyHandle;
	EOS_HLobbySearch LobbySearchByLobbyIDHandle;
	EOS_HLobbySearch LobbySearchByUserIDHandle;
	EOS_NotificationId OnLobbyMemberStatusUpdateID;

	UPROPERTY() class UOnlineUserSubsystem* OnlineUserSubsystem;
	UPROPERTY() class ULocalUserSubsystem* LocalUserSubsystem;
	UPROPERTY() ULocalUser* LocalUser;
	
	TArray<FString> UsersToLoad; // Used to check if user's have left after loading their data.
	FLobbyDetails LobbyDetails; // Struct containing all the necessary data.

public:
	void LoadLobbyDetails(const EOS_LobbyId LobbyID, TFunction<void(bool bSuccess, const FString& ErrorCode)> OnCompleteCallback);
	FORCEINLINE TMap<FString, UEosUser*> GetMemberMap() const {return LobbyDetails.MemberList;}
	TArray<UEosUser*> GetMemberList() const;
	UEosUser* GetMember(const EOS_ProductUserId ProductUserID);
	void StoreMember(UEosUser* EosUser);
	void StoreMembers(TArray<UEosUser*> &EosUsers);




	
	/*
	 * Shadow lobby
	 */
	void CreateShadowLobby();
	void OnCreateShadowLobbyComplete(const uint64 ShadowLobbyID);
	
	void JoinShadowLobby(const uint64 ShadowLobbyID);
	void OnJoinShadowLobbyComplete(const uint64 ShadowLobbyID);

	void AddShadowLobbyIdAttribute(const uint64 ShadowLobbyID);
	static void OnAddShadowLobbyIdAttributeComplete(const EOS_Lobby_UpdateLobbyCallbackInfo* Data);

private:
	FDelegateHandle OnCreateShadowLobbyCompleteDelegateHandle;
	FDelegateHandle OnJoinShadowLobbyCompleteDelegateHandle;
	
	UPROPERTY() class USteamLobbySubsystem* SteamLobbySubsystem;
};
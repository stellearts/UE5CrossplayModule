// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "eos_sdk.h"
#include "OnlineMultiplayer_CommonTypes.h"
#include "LobbySubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogLobbySubsystem, Log, All);
inline DEFINE_LOG_CATEGORY(LogLobbySubsystem);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCreateLobbyCompleteDelegate, const bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnJoinLobbyCompleteDelegate, const bool, bSuccess);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLobbyUserJoinedDelegate, const FOnlineUser, OnlineUser);
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
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

public:
	UFUNCTION(BlueprintCallable, Category = "Online|Lobby")
	void CreateLobby();
	void JoinLobbyByID(const EOS_LobbyId LobbyID);
	void JoinLobbyByUserID(const EOS_ProductUserId UserID);

private:
	void JoinLobbyByHandle(const EOS_HLobbyDetails LobbyDetailsHandle);
	
	static void OnCreateLobbyComplete(const EOS_Lobby_CreateLobbyCallbackInfo* Data);
	static void OnJoinLobbyComplete(const EOS_Lobby_JoinLobbyCallbackInfo* Data);
	
	static void OnLobbyUpdated(const EOS_Lobby_UpdateLobbyCallbackInfo* Data);
	static void OnLobbyMemberStatusUpdate(const EOS_Lobby_LobbyMemberStatusReceivedCallbackInfo* Data);

	void OnLobbyUserJoined(const EOS_ProductUserId TargetUserID);
	void OnLobbyUserLeft(const EOS_ProductUserId TargetUserID);
	void OnLobbyUserDisconnected(const EOS_ProductUserId TargetUserID);
	void OnLobbyUserKicked(const EOS_ProductUserId TargetUserID);
	void OnLobbyUserPromoted(const EOS_ProductUserId TargetUserID);
	
	class FEosManager* EosManager;
	EOS_HLobby LobbyHandle;
	EOS_HLobbySearch LobbySearchByLobbyIDHandle;
	EOS_HLobbySearch LobbySearchByUserIDHandle;
	EOS_NotificationId OnLobbyMemberStatusUpdateID;

	UPROPERTY()
	class UUserStateSubsystem* LocalUserState;

	// Determines whether the user is still in the lobby after loading their data.
	TArray<EOS_ProductUserId> UsersToLoad;

public:
	// Delegates for other classes to bind to.
	
	// Create/Join Lobby delegates.
	UPROPERTY(BlueprintAssignable, Category = "Lobby|Delegates")
	FOnCreateLobbyCompleteDelegate OnCreateLobbyCompleteDelegate;
	UPROPERTY(BlueprintAssignable, Category = "Lobby|Delegates")
	FOnJoinLobbyCompleteDelegate OnJoinLobbyCompleteDelegate;
	
	// Lobby-User update delegates.
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

	


	
	// ------------------------------- Shadow Lobby -------------------------------
	 
	
	void CreateShadowLobby();
	void OnCreateShadowLobbyComplete(const uint64 ShadowLobbyID);
	
	void JoinShadowLobby(const uint64 ShadowLobbyID);
	void OnJoinShadowLobbyComplete(const uint64 ShadowLobbyID);

	void AddShadowLobbyIdAttribute(const uint64 ShadowLobbyID);
	static void OnAddShadowLobbyIdAttributeComplete(const EOS_Lobby_UpdateLobbyCallbackInfo* Data);

private:
	// Shadow Lobby delegate-handles for unbinding upon completion.
	FDelegateHandle OnCreateShadowLobbyCompleteDelegateHandle;
	FDelegateHandle OnJoinShadowLobbyCompleteDelegateHandle;

	UPROPERTY()
	class USteamLobbySubsystem* SteamLobbySubsystem;
};
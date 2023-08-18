// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "eos_sdk.h"
#include "Types/UserTypes.h"
#include "Types/LobbyTypes.h"
#include "LobbySubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogLobbySubsystem, Log, All);
inline DEFINE_LOG_CATEGORY(LogLobbySubsystem);



DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCreateLobbyCompleteDelegate, const ECreateLobbyResultCode, const FLobby&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnJoinLobbyCompleteDelegate, const EJoinLobbyResultCode, const FLobby&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLeaveLobbyCompleteDelegate, const ELeaveLobbyResultCode);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyUserJoinedDelegate, const UOnlineUser*);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyUserLeftDelegate, const FString& ProductUserID);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyUserDisconnectedDelegate, const FString& ProductUserID);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyUserKickedDelegate, const FString& ProductUserID);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyUserPromotedDelegate, const FString& ProductUserID);

DECLARE_MULTICAST_DELEGATE(FOnGameStartDelegate);
DECLARE_MULTICAST_DELEGATE(FOnGameEndDelegate);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyAttributeChanged, const FLobbyAttribute&);


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

	void SetAttribute(const FLobbyAttribute& Attribute);
	void SetAttributes(TArray<FLobbyAttribute> Attributes);
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
	
	FOnLobbyAttributeChanged OnLobbyAttributeChanged;

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
	
	UPROPERTY() FLobby Lobby;
	void LoadLobbyDetails(const EOS_LobbyId LobbyID, TFunction<void(bool bSuccess)> OnCompleteCallback);

	TArray<FString> UsersToLoad; // Used to check if user's have left after loading their data.
	TArray<FString> SpecialAttributes{"GameStarted", "SteamLobbyID", "PsnLobbyID", "XboxLobbyID"};

public:
	FORCEINLINE const FLobby& GetLobby() const { return Lobby; }
	FORCEINLINE bool InLobby() const { return !Lobby.ID.IsEmpty(); }




	
	/*
	 * Shadow lobby
	 */

	void OnCreateShadowLobbyComplete(const struct FShadowLobbyResult &ShadowLobbyResult);
	
	void JoinShadowLobby(const uint64 ShadowLobbyID);
	void OnJoinShadowLobbyComplete(const uint64 ShadowLobbyID);

	void AddShadowLobbyIDAttribute(const FString& ShadowLobbyID);
};
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

DECLARE_MULTICAST_DELEGATE_OneParam(FOnSessionIDAttributeAdded, const FString&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyAttributeChanged, const FLobbyAttribute&);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyStartedDelegate, const FString& ServerAddress);
DECLARE_MULTICAST_DELEGATE(FOnLobbyStoppedDelegate);

USTRUCT(BlueprintType)
struct FLatentActionInfos
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite)
	FLatentActionInfo OnSuccess;

	UPROPERTY(BlueprintReadWrite)
	FLatentActionInfo OnFailure;
};

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
	virtual void Deinitialize() override;

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
	
	FOnSessionIDAttributeAdded OnSessionIDAttributeChanged; // For joining a session
	FOnLobbyAttributeChanged OnLobbyAttributeChanged; // Custom lobby attribute

	FOnLobbyStartedDelegate OnLobbyStartedDelegate;
	FOnLobbyStoppedDelegate OnLobbyStoppedDelegate;

private:
	FDelegateHandle StartServerCompleteDelegateHandle;
	
public:
	void CreateLobby(const int32 MaxMembers);
	void JoinLobbyByID(const FString& LobbyID);
	void JoinLobbyByUserID(const FString& UserID);
	void LeaveLobby();

	UFUNCTION(BlueprintCallable, meta = (Latent, WorldContext = "WorldContextObject", LatentInfo = "LatentInfos"))
	void StartListenServer(UObject* WorldContextObject, FLatentActionInfos LatentInfos);

private:
	void JoinLobbyByHandle(const EOS_HLobbyDetails& LobbyDetailsHandle);
	static void OnJoinLobbyComplete(const EOS_Lobby_JoinLobbyCallbackInfo* Data);

public:
	FORCEINLINE void SetAttribute(const FLobbyAttribute& Attribute, TFunction<void(const bool bWasSuccessful)> OnCompleteCallback) { SetAttributes(TArray<FLobbyAttribute>{Attribute}, OnCompleteCallback); }
	void SetAttributes(TArray<FLobbyAttribute> Attributes, TFunction<void(const bool bWasSuccessful)> OnCompleteCallback);

private:
	TArray<FLobbyAttribute> FilterAttributes(TArray<FLobbyAttribute>& Attributes);
	TArray<FLobbyAttribute> AddAttributeOnModificationHandle(EOS_HLobbyModification& LobbyModificationHandle, const TArray<FLobbyAttribute>& Attributes);
	TArray<FLobbyAttribute> GetAttributesFromDetailsHandle();
	
	static void OnLobbyUpdate(const EOS_Lobby_LobbyUpdateReceivedCallbackInfo* Data);
	static void OnLobbyMemberStatusUpdate(const EOS_Lobby_LobbyMemberStatusReceivedCallbackInfo* Data);
	void OnLobbyUserJoined(const FString& TargetUserID);
	void OnLobbyUserLeft(const FString& TargetUserID);
	void OnLobbyUserDisconnected(const FString& TargetUserID);
	void OnLobbyUserKicked(const FString& TargetUserID);
	void OnLobbyUserPromoted(const FString& TargetUserID);
	
	// EOS Variables
	EOS_HLobby LobbyHandle;
	EOS_HLobbyDetails GetLobbyDetailsHandle() const;
	EOS_HLobbySearch LobbySearchByLobbyIDHandle;
	EOS_HLobbySearch LobbySearchByUserIDHandle;
	EOS_NotificationId OnLobbyUpdateNotification;
	EOS_NotificationId OnLobbyMemberStatusNotification;

	// Subsystems
	class FEosManager* EosManager;
	UPROPERTY() class ULocalUserSubsystem* LocalUserSubsystem;
	UPROPERTY() class UOnlineUserSubsystem* OnlineUserSubsystem;
	UPROPERTY() class UPlatformLobbySubsystemBase* LocalPlatformLobbySubsystem;
	
	UPROPERTY() FLobby Lobby;
	void LoadLobby(TFunction<void(bool bSuccess)> OnCompleteCallback);

	TArray<FString> UsersToLoad; // Used to check if user's have left after loading their data.
	TArray<FString> SpecialAttributes{"ServerAddress", "SessionID", "SteamLobbyID", "PsnLobbyID", "XboxLobbyID"};

public:
	FORCEINLINE FLobby& GetLobby() { return Lobby; }
	FORCEINLINE bool ActiveLobby() const { return !Lobby.ID.IsEmpty(); }




	
	/*
	 * Shadow lobby
	 */

private:
	void OnCreateShadowLobbyComplete(const struct FShadowLobbyResult &ShadowLobbyResult);
	
	void JoinShadowLobby(const uint64 ShadowLobbyID);
	void OnJoinShadowLobbyComplete(const uint64 ShadowLobbyID);

	void AddShadowLobbyIDAttribute(const FString& ShadowLobbyID); // TODO: Add Shadow Lobby Type, currently only steam.
};
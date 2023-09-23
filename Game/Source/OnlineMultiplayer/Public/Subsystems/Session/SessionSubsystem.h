// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "eos_sdk.h"
#include "Types/SessionTypes.h"
#include "SessionSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSessionSubsystem, Log, All);
inline DEFINE_LOG_CATEGORY(LogSessionSubsystem);



DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCreateSessionCompleteDelegate, const ECreateSessionResultCode, const FSession&);
DECLARE_MULTICAST_DELEGATE(FOnServerCreatedDelegate);



/**
 * Subsystem for managing sessions.
 * CURRENTLY UNUSED.
 */
UCLASS(BlueprintType)
class ONLINEMULTIPLAYER_API USessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	FOnCreateSessionCompleteDelegate OnCreateSessionCompleteDelegate;
	FOnServerCreatedDelegate OnServerCreatedDelegate;

private:
	FDelegateHandle OnServerCreatedDelegateHandle;

public:
	UFUNCTION(BlueprintCallable, Category = "Online|Session")
	void CreateSession(const FSessionSettings& Settings);
	void StartSession();
	void EndSession();

private:
	void JoinSessionByHandle(const EOS_HSessionDetails& DetailsHandle);
	static void OnJoinSessionComplete(const EOS_Sessions_JoinSessionCallbackInfo* Data);
	
public:

	void InvitePlayer(const FString& ProductUserID);

private:
	static void OnInviteReceived(const EOS_Sessions_SessionInviteReceivedCallbackInfo* Data);
	
	TArray<FSessionAttribute> FilterAttributes(TArray<FSessionAttribute> Attributes);
	bool AddAttributeToHandle(EOS_HSessionModification& Handle, const FSessionAttribute& Attribute);

public:
	FORCEINLINE void SetAttribute(const FSessionAttribute& Attribute) { SetAttributes(TArray<FSessionAttribute>{Attribute}); }
	void SetAttributes(const TArray<FSessionAttribute>& Attributes);
	void SetSpecialAttribute(const FSessionAttribute& Attribute, const TFunction<void(bool bWasSuccessful)>& Callback);

private:
	// EOS Variables
	EOS_HSessions SessionHandle;
	EOS_HActiveSession GetActiveSessionHandle() const;
	EOS_HSessionDetails SessionDetailsHandle;
	EOS_NotificationId OnSessionInviteNotification;

	class FEosManager* EosManager;
	UPROPERTY() class ULocalUserSubsystem* LocalUserSubsystem;
	UPROPERTY() class ULobbySubsystem* LobbySubsystem;

	UPROPERTY() FSession Session;
	void LoadSession(TFunction<void(bool bSuccess)> OnCompleteCallback);

	TArray<FString> SpecialAttributes{"GameStarted"};

public:
	FORCEINLINE const FSession& GetSession() const { return Session; }
	FORCEINLINE bool ActiveSession() const { return !Session.ID.IsEmpty(); }
};

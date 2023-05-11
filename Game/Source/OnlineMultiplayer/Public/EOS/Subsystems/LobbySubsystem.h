// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "eos_sdk.h"
#include "Steam/SteamLobbyManager.h"
#include "LobbySubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogLobbySubsystem, Log, All);
inline DEFINE_LOG_CATEGORY(LogLobbySubsystem);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUserJoinLobby, const FString&, UserId);



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

private:
	void JoinLobby(const EOS_HLobbyDetails LobbyDetailsHandle);
	
	static void OnCreateLobbyComplete(const EOS_Lobby_CreateLobbyCallbackInfo* Data);
	static void OnJoinLobbyComplete(const EOS_Lobby_JoinLobbyCallbackInfo* Data);
	static void OnLobbySearchComplete(const EOS_LobbySearch_FindCallbackInfo* Data);
	static void OnLobbyUpdated(const EOS_Lobby_UpdateLobbyCallbackInfo* Data);

	void OnCreateShadowLobbyComplete(const uint64 ShadowLobbyId);
	FDelegateHandle CreateShadowLobbyCompleteDelegateHandle;

	
	void ApplySteamInviterAttributeModification(const EOS_LobbyId LobbyId);
	
	class FEosManager* EosManager;
	TSharedPtr<class FLocalUserState> LocalUserState;
	EOS_HLobby LobbyHandle;
	EOS_HLobbySearch LobbySearchHandle;

	TUniquePtr<FSteamLobbyManager> SteamLobbyManager;
	bool bInLobby = false;
	bool bInShadowLobby = false;
};

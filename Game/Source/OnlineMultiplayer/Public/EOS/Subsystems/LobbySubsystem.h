// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "eos_sdk.h"
#include "LobbySubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogLobbySubsystem, Log, All);
inline DEFINE_LOG_CATEGORY(LogLobbySubsystem);



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
	static void OnCreateLobbyComplete(const EOS_Lobby_CreateLobbyCallbackInfo* Data);
	static void OnLobbyUpdated(const EOS_Lobby_UpdateLobbyCallbackInfo* Data);
	
	void ApplySteamInviterAttributeModification(const EOS_LobbyId LobbyId);
	
	class FEosManager* EosManager;
	EOS_HLobby LobbyHandle;
};

// Copyright © 2023 Melvin Brink

#include "Platforms/Steam/SteamLobbyManager.h"
#pragma warning(push)
#pragma warning(disable: 4996)
#pragma warning(disable: 4265)
#include "steam_api.h"
#include "steamnetworkingtypes.h"
#pragma warning(pop)
#include "Platforms/EOS/Subsystems/LobbySubsystem.h"
#include "UserStateSubsystem.h"



FSteamLobbyManager::FSteamLobbyManager(UGameInstance* InGameInstance)
{
	GameInstance = InGameInstance;
}

/**
 * Creates a Steam lobby and set the EOS lobby ID as lobby data.
 *
 * Steam users can then join the EOS lobby by joining this shadow lobby.
 *
 * Purpose is to allow Steam users to join the EOS lobby through the Steam overlay, and also update lobby presence in the overlay.
 *
 * This function should be called by the first Steam user that joins the EOS lobby, and he will be the owner of this Steam lobby.
 */
void FSteamLobbyManager::CreateShadowLobby()
{
	if(!GameInstance) return;
	const UUserStateSubsystem* LocalUserState = GameInstance->GetSubsystem<UUserStateSubsystem>();
	if(!LocalUserState) return;

	if(!LocalUserState->IsInLobby())
	{
		UE_LOG(LogSteamLobbyManager, Warning, TEXT("Not in an EOS-Lobby. Cannot create Shadow-Lobby without being in an EOS-Lobby."));
		return;
	}
	
	const SteamAPICall_t SteamCreateShadowLobbyAPICall = SteamMatchmaking()->CreateLobby(k_ELobbyTypeFriendsOnly, 4);
	OnCreateShadowLobbyCallResult.Set(SteamCreateShadowLobbyAPICall, this, &FSteamLobbyManager::OnCreateShadowLobbyComplete);
}

void FSteamLobbyManager::OnCreateShadowLobbyComplete(LobbyCreated_t* Data, bool bIOFailure)
{
	UUserStateSubsystem* LocalUserState = GameInstance->GetSubsystem<UUserStateSubsystem>();
	if(!LocalUserState) return;
	LocalUserState->SetShadowLobbyID(Data->m_ulSteamIDLobby); // 0 if failed to create lobby.
	
	if(Data->m_eResult == k_EResultOK)
	{
		UE_LOG(LogSteamLobbyManager, Log, TEXT("Shadow-Lobby created!"));
		
		// Set the lobby data to include the EOS lobby ID. This will allow Steam users to join the EOS lobby through the shadow lobby.
		SteamMatchmaking()->SetLobbyData(Data->m_ulSteamIDLobby, "EOSLobbyID", LocalUserState->GetLobbyID());
	}
	else
	{
		UE_LOG(LogSteamLobbyManager, Log, TEXT("Failed to create Shadow-Lobby with Result Code: %d"), Data->m_eResult);
		OnCreateShadowLobbyCompleteDelegate.Broadcast(0);
	}
}

// TODO: can be removed probably.
void FSteamLobbyManager::OnLobbyDataUpdateComplete(LobbyDataUpdate_t* Data)
{
	// TODO: Add some checks to make sure the user wanted to create a lobby and listens for the result.
	if(Data->m_bSuccess)
	{
		OnCreateShadowLobbyCompleteDelegate.Broadcast(Data->m_ulSteamIDLobby);
	}
	else
	{
		// Should only be false if RequestLobbyData() was called on a lobby that no longer exists, according to the Steam api.
		OnCreateShadowLobbyCompleteDelegate.Broadcast(0);
	}
}


// --------------------------------------------


void FSteamLobbyManager::JoinShadowLobby(uint64 SteamLobbyID)
{
	const SteamAPICall_t SteamJoinShadowLobbyAPICall = SteamMatchmaking()->JoinLobby(SteamLobbyID);
	OnShadowLobbyEnterCallResult.Set(SteamJoinShadowLobbyAPICall, this, &FSteamLobbyManager::OnJoinShadowLobbyComplete);
}

void FSteamLobbyManager::OnJoinShadowLobbyComplete(LobbyEnter_t* Data, bool bIOFailure)
{
	if(!GameInstance) return;
	UUserStateSubsystem* LocalUserState = GameInstance->GetSubsystem<UUserStateSubsystem>();
	ULobbySubsystem* LobbySubsystem = GameInstance->GetSubsystem<ULobbySubsystem>();
	if(!LocalUserState || !LobbySubsystem) return;
	LocalUserState->SetShadowLobbyID(Data->m_ulSteamIDLobby); // 0 if failed to join.
	
	if(Data->m_EChatRoomEnterResponse == k_EChatRoomEnterResponseSuccess)
	{
		UE_LOG(LogSteamLobbyManager, Log, TEXT("Shadow-Lobby joined!"));

		// Join the EOS lobby if the user is not already in the EOS lobby.
		if(!LocalUserState->IsInLobby())
		{
			const char* EosLobbyID = SteamMatchmaking()->GetLobbyData(Data->m_ulSteamIDLobby, "EOSLobbyID");
			LobbySubsystem->JoinLobbyByID(EosLobbyID);
		}
	}
	else
		UE_LOG(LogSteamLobbyManager, Log, TEXT("Failed to join Shadow-Lobby!"));
}


// --------------------------------------------


void FSteamLobbyManager::OnJoinShadowLobbyRequest(GameLobbyJoinRequested_t* Data)
{
	CSteamID FriendId = Data->m_steamIDFriend;
	
}

void FSteamLobbyManager::OnJoinRichPresenceRequest(GameRichPresenceJoinRequested_t* Data)
{
	CSteamID FriendId = Data->m_steamIDFriend;
	
}
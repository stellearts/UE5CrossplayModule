// Copyright © 2023 Melvin Brink

#include "Steam/SteamLobbyManager.h"

#pragma warning(push)
#pragma warning(disable: 4996)
#pragma warning(disable: 4265)
#include "steam_api.h"
#include "isteamuser.h"
#include "steamnetworkingtypes.h"
#pragma warning(pop)



/**
 * Creates a Steam lobby with the EOS lobby ID as lobby data.
 *
 * Steam users can then also join the EOS lobby by joining this shadow lobby.
 *
 * This will also allow the lobby-presence to show up in the Steam overlay and allow users to join the lobby through the overlay.
 */
void FSteamLobbyManager::CreateShadowLobby(const char* EosLobbyID)
{
	const SteamAPICall_t SteamCreateShadowLobbyAPICall = SteamMatchmaking()->CreateLobby(k_ELobbyTypeFriendsOnly, 4);

	// Set the lobby data to include the EOS lobby ID.
	SteamMatchmaking()->SetLobbyData(SteamCreateShadowLobbyAPICall, "EOSLobbyID", EosLobbyID);

	// Set the call result to trigger OnShadowLobbyCreated when the lobby is created.
	OnCreateShadowLobbyCallResult.Set(SteamCreateShadowLobbyAPICall, this, &FSteamLobbyManager::OnCreateShadowLobbyComplete);
}

void FSteamLobbyManager::OnCreateShadowLobbyComplete(LobbyCreated_t* Data, bool bIOFailure)
{
	if(Data->m_eResult == k_EResultOK)
	{
		UE_LOG(LogSteamLobbyManager, Log, TEXT("Lobby created!"));
		OnCreateShadowLobbyCompleteDelegate.Broadcast(Data->m_ulSteamIDLobby);
	}
	else
	{
		UE_LOG(LogSteamLobbyManager, Log, TEXT("Failed to create lobby!"));
		OnCreateShadowLobbyCompleteDelegate.Broadcast(0);
	}
}


// --------------------------------------------


void FSteamLobbyManager::OnShadowLobbyEntered(LobbyEnter_t* Data, bool bIOFailure)
{
	if(Data->m_EChatRoomEnterResponse == k_EChatRoomEnterResponseSuccess)
	{
		UE_LOG(LogSteamLobbyManager, Log, TEXT("Shadow lobby joined!"));

		// Get the EOS lobby ID from the lobby data
		const char* EOSLobbyID = SteamMatchmaking()->GetLobbyData(Data->m_ulSteamIDLobby, "EOSLobbyID");

		// Redirect the user to the EOS lobby using the lobby ID.
	}
	else
	{
		UE_LOG(LogSteamLobbyManager, Log, TEXT("Failed to join shadow lobby!"));
	}
}

void FSteamLobbyManager::OnJoinShadowLobbyRequest(GameLobbyJoinRequested_t* Data)
{
	CSteamID FriendId = Data->m_steamIDFriend;
	
}

void FSteamLobbyManager::OnJoinRichPresenceRequest(GameRichPresenceJoinRequested_t* Data)
{
	CSteamID FriendId = Data->m_steamIDFriend;
	
}
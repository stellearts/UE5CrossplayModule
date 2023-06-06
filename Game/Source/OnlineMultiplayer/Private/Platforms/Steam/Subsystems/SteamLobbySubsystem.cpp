// Copyright © 2023 Melvin Brink

#include "Platforms/Steam/Subsystems/SteamLobbySubsystem.h"

#pragma warning(push)
#pragma warning(disable: 4996)
#pragma warning(disable: 4265)
#include "steam_api.h"
#include "steamnetworkingtypes.h"
#pragma warning(pop)

#include "Platforms/EOS/Subsystems/LobbySubsystem.h"
#include "Platforms/EOS/Subsystems/LocalUserSubsystem.h"


void USteamLobbySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}


// --------------------------------------------


/**
 * Creates a Steam lobby and set the EOS lobby ID as lobby data.
 *
 * Steam users can then join the EOS lobby by joining this shadow lobby.
 *
 * Purpose is to allow Steam users to join the EOS lobby through the Steam overlay, and also update lobby presence in the overlay.
 *
 * This function should be called by the first Steam user that joins the EOS lobby, and he will be the owner of this Steam lobby.
 */
void USteamLobbySubsystem::CreateShadowLobby()
{
	const ULocalUserSubsystem* LocalUserSubsystem = GetGameInstance()->GetSubsystem<ULocalUserSubsystem>();
	if(!LocalUserSubsystem) return;

	if(!LocalUserSubsystem->GetLocalUser()->IsInLobby())
	{
		UE_LOG(LogSteamLobbySubsystem, Warning, TEXT("Not in an EOS-Lobby. Cannot create Shadow-Lobby without being in an EOS-Lobby."));
		return;
	}
	
	const SteamAPICall_t SteamCreateShadowLobbyAPICall = SteamMatchmaking()->CreateLobby(k_ELobbyTypeFriendsOnly, 4);
	OnCreateShadowLobbyCallResult.Set(SteamCreateShadowLobbyAPICall, this, &USteamLobbySubsystem::OnCreateShadowLobbyComplete);
}

void USteamLobbySubsystem::OnCreateShadowLobbyComplete(LobbyCreated_t* Data, bool bIOFailure)
{
	const ULocalUserSubsystem* LocalUserSubsystem = GetGameInstance()->GetSubsystem<ULocalUserSubsystem>();
	if(!LocalUserSubsystem) return;
	LocalUserSubsystem->GetLocalUser()->SetShadowLobbyID(Data->m_ulSteamIDLobby); // 0 if failed to create lobby.
	
	if(Data->m_eResult == k_EResultOK)
	{
		UE_LOG(LogSteamLobbySubsystem, Log, TEXT("Shadow-Lobby created!"));
		
		// Set the lobby data to include the EOS lobby ID. This will allow Steam users to join the EOS lobby through the shadow lobby.
		SteamMatchmaking()->SetLobbyData(Data->m_ulSteamIDLobby, "EOSLobbyID", LocalUserSubsystem->GetLocalUser()->GetLobbyID().c_str());
	}
	else
	{
		UE_LOG(LogSteamLobbySubsystem, Log, TEXT("Failed to create Shadow-Lobby with Result Code: %d"), Data->m_eResult);
		OnCreateShadowLobbyCompleteDelegate.Broadcast(0);
	}
}

// TODO: can be removed probably.
void USteamLobbySubsystem::OnLobbyDataUpdateComplete(LobbyDataUpdate_t* Data)
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


void USteamLobbySubsystem::JoinShadowLobby(uint64 SteamLobbyID)
{
	const SteamAPICall_t SteamJoinShadowLobbyAPICall = SteamMatchmaking()->JoinLobby(SteamLobbyID);
	OnShadowLobbyEnterCallResult.Set(SteamJoinShadowLobbyAPICall, this, &USteamLobbySubsystem::OnJoinShadowLobbyComplete);
}

void USteamLobbySubsystem::OnJoinShadowLobbyComplete(LobbyEnter_t* Data, bool bIOFailure)
{
	const ULocalUserSubsystem* LocalUserSubsystem = GetGameInstance()->GetSubsystem<ULocalUserSubsystem>();
	ULobbySubsystem* LobbySubsystem = GetGameInstance()->GetSubsystem<ULobbySubsystem>();
	if(!LocalUserSubsystem || !LobbySubsystem) return;
	LocalUserSubsystem->GetLocalUser()->SetShadowLobbyID(Data->m_ulSteamIDLobby); // 0 if failed to join.
	
	if(Data->m_EChatRoomEnterResponse == k_EChatRoomEnterResponseSuccess)
	{
		UE_LOG(LogSteamLobbySubsystem, Log, TEXT("Shadow-Lobby joined!"));

		// Join the EOS lobby if the user is not already in the EOS lobby.
		if(!LocalUserSubsystem->GetLocalUser()->IsInLobby())
		{
			const char* EosLobbyID = SteamMatchmaking()->GetLobbyData(Data->m_ulSteamIDLobby, "EOSLobbyID");
			LobbySubsystem->JoinLobbyByID(EosLobbyID);
		}
	}
	else
		UE_LOG(LogSteamLobbySubsystem, Log, TEXT("Failed to join Shadow-Lobby!"));
}


// --------------------------------------------


void USteamLobbySubsystem::OnJoinShadowLobbyRequest(GameLobbyJoinRequested_t* Data)
{
	CSteamID FriendId = Data->m_steamIDFriend;
	
}

void USteamLobbySubsystem::OnJoinRichPresenceRequest(GameRichPresenceJoinRequested_t* Data)
{
	CSteamID FriendId = Data->m_steamIDFriend;
	
}

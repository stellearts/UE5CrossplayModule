// Copyright © 2023 Melvin Brink

#include "Subsystems/Lobby/SteamLobbySubsystem.h"
#include "Subsystems/Lobby/LobbySubsystem.h"
#include "Subsystems/User/Local/LocalUserSubsystem.h"

#pragma warning(push)
#pragma warning(disable: 4996)
#pragma warning(disable: 4265)
#include "steam_api.h"
#include "steamnetworkingtypes.h"
#pragma warning(pop)



void USteamLobbySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LocalUserSubsystem = Collection.InitializeDependency<ULocalUserSubsystem>();
	LobbySubsystem = Collection.InitializeDependency<ULobbySubsystem>();
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
void USteamLobbySubsystem::CreateLobby()
{
	if(!LobbySubsystem->ActiveLobby())
	{
		UE_LOG(LogSteamLobbySubsystem, Warning, TEXT("Not in an EOS-lobby. Cannot create Shadow-Lobby without being in an EOS-lobby."));
		return;
	}

	// If already in a Steam shadow-lobby, check if the EOSLobbyID in the lobby-data is the same to confirm that the lobby is correct.
	if(InLobby())
	{
		const FString EosLobbyID = FString(SteamMatchmaking()->GetLobbyData(FCString::Strtoui64(*LobbyDetails.LobbyID, nullptr, 10), "EOSLobbyID"));
		if(LobbyDetails.LobbyID != EosLobbyID)
		{
			// Leave this lobby and create a new one which will set the correct EOSLobbyID attribute upon completion.
			UE_LOG(LogSteamLobbySubsystem, Warning, TEXT("The EOS-Lobby-ID of the EOS-Lobby and the EOS-Lobby-ID in the Shadow-Lobby attributes are not the same, leaving current Shadow-Lobby and creating a new one with the correct attributes."))
			LeaveLobby();
			CreateLobby();
		}
		else UE_LOG(LogSteamLobbySubsystem, Warning, TEXT("Already in a shadow-lobby"));
		
		return;
	}
	
	const SteamAPICall_t SteamCreateShadowLobbyAPICall = SteamMatchmaking()->CreateLobby(k_ELobbyTypeFriendsOnly, 4);
	OnCreateShadowLobbyCallResult.Set(SteamCreateShadowLobbyAPICall, this, &USteamLobbySubsystem::OnCreateLobbyComplete);
}

void USteamLobbySubsystem::OnCreateLobbyComplete(LobbyCreated_t* Data, bool bIOFailure)
{
	LobbyDetails.LobbyID = FString::Printf(TEXT("%llu"), Data->m_ulSteamIDLobby);
	
	if(Data->m_eResult == k_EResultOK)
	{
		UE_LOG(LogSteamLobbySubsystem, Log, TEXT("Shadow-Lobby created."));

		// Set the lobby data to include the EOS lobby ID. This will allow Steam users to join the EOS lobby through the shadow lobby
		SteamMatchmaking()->SetLobbyData(Data->m_ulSteamIDLobby, "EOSLobbyID", TCHAR_TO_ANSI(*LobbySubsystem->GetLobby().ID));
		UE_LOG(LogSteamLobbySubsystem, Log, TEXT("SteamMatchmaking()->SetLobbyData: %s"), *LobbySubsystem->GetLobby().ID);
		
		OnCreateShadowLobbyCompleteDelegate.ExecuteIfBound(FShadowLobbyResult{LobbyDetails, EShadowLobbyResultCode::Success});
	}
	else
	{
		UE_LOG(LogSteamLobbySubsystem, Log, TEXT("Failed to create Shadow-Lobby with Result Code: %d"), Data->m_eResult);
		OnCreateShadowLobbyCompleteDelegate.ExecuteIfBound(FShadowLobbyResult{LobbyDetails, EShadowLobbyResultCode::CreateFailure});
	}
}

void USteamLobbySubsystem::OnLobbyDataUpdateComplete(LobbyDataUpdate_t* Data)
{
	if(Data->m_bSuccess)
	{
		UE_LOG(LogTemp, Warning, TEXT("OnLobbyDataUpdateComplete Success"));
	}
	else
	{
		// Should only be false if RequestLobbyData() was called on a lobby that no longer exists, according to the Steam api.
		UE_LOG(LogTemp, Warning, TEXT("OnLobbyDataUpdateComplete Fail"));
	}
}

void USteamLobbySubsystem::JoinLobby(const FString& LobbyID)
{
	const SteamAPICall_t SteamJoinShadowLobbyAPICall = SteamMatchmaking()->JoinLobby(FCString::Strtoui64(*LobbyID, nullptr, 10));
	OnShadowLobbyEnterCallResult.Set(SteamJoinShadowLobbyAPICall, this, &USteamLobbySubsystem::OnJoinLobbyComplete);
}

void USteamLobbySubsystem::OnJoinLobbyComplete(LobbyEnter_t* Data, bool bIOFailure)
{
	if(!LocalUserSubsystem || !LobbySubsystem) return;
	LobbyDetails.LobbyID = Data->m_ulSteamIDLobby == 0 ? FString("") : FString::Printf(TEXT("%llu"), Data->m_ulSteamIDLobby); // '0' if failed to join.
	
	if(Data->m_EChatRoomEnterResponse == k_EChatRoomEnterResponseSuccess)
	{
		UE_LOG(LogSteamLobbySubsystem, Log, TEXT("Shadow-lobby joined."));

		// Join the EOS lobby if the user is not already in the EOS lobby.
		if(!LobbySubsystem->ActiveLobby())
		{
			const FString EosLobbyID = FString(SteamMatchmaking()->GetLobbyData(Data->m_ulSteamIDLobby, "EOSLobbyID"));
			UE_LOG(LogSteamLobbySubsystem, Log, TEXT("Trying to joing EOS Lobby: [%s]"), *EosLobbyID);
			LobbySubsystem->JoinLobbyByID(EosLobbyID);
		}
	}
	else
		UE_LOG(LogSteamLobbySubsystem, Log, TEXT("Failed to join Shadow-lobby. m_EChatRoomEnterResponse: [%i]"), Data->m_EChatRoomEnterResponse);
}

void USteamLobbySubsystem::LeaveLobby()
{
	SteamMatchmaking()->LeaveLobby(FCString::Strtoui64(*LobbyDetails.LobbyID, nullptr, 10));
	LobbyDetails.Reset();
	UE_LOG(LogSteamLobbySubsystem, Log, TEXT("Left the shadow-lobby."));
}


// --------------------------------------------


/**
 * Called when the user tries to join a lobby from their friends list.
 */
void USteamLobbySubsystem::OnJoinLobbyRequest(GameLobbyJoinRequested_t* Data)
{
	// TODO: If the game isn't running yet then the game will be automatically launched with the command line parameter +connect_lobby <64-bit lobby Steam ID> instead.
	if(Data && Data->m_steamIDLobby.IsValid()) JoinLobby(FString::Printf(TEXT("%llu"), Data->m_steamIDLobby.ConvertToUint64()));
}

/**
 * Called when the user tries to join a game from their friends list.
 */
void USteamLobbySubsystem::OnJoinRichPresenceRequest(GameRichPresenceJoinRequested_t* Data)
{
	UE_LOG(LogSteamLobbySubsystem, Log, TEXT("Joining lobby through rich presence..."));
}
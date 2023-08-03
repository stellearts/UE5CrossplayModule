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
void USteamLobbySubsystem::CreateShadowLobby()
{
	if(!LobbySubsystem->InLobby())
	{
		UE_LOG(LogSteamLobbySubsystem, Warning, TEXT("Not in an EOS-lobby. Cannot create Shadow-Lobby without being in an EOS-lobby."));
		return;
	}
	
	const SteamAPICall_t SteamCreateShadowLobbyAPICall = SteamMatchmaking()->CreateLobby(k_ELobbyTypeFriendsOnly, 4);
	OnCreateShadowLobbyCallResult.Set(SteamCreateShadowLobbyAPICall, this, &USteamLobbySubsystem::OnCreateShadowLobbyComplete);
}

void USteamLobbySubsystem::OnCreateShadowLobbyComplete(LobbyCreated_t* Data, bool bIOFailure)
{
	LobbyDetails.LobbyID = FString::Printf(TEXT("%llu"), Data->m_ulSteamIDLobby);
	
	if(Data->m_eResult == k_EResultOK)
	{
		UE_LOG(LogSteamLobbySubsystem, Log, TEXT("Shadow-Lobby created."));
		
		// Set the lobby data to include the EOS lobby ID. This will allow Steam users to join the EOS lobby through the shadow lobby.
		SteamMatchmaking()->SetLobbyData(Data->m_ulSteamIDLobby, "EOSLobbyID", TCHAR_TO_ANSI(*LobbyDetails.LobbyID));
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


void USteamLobbySubsystem::JoinShadowLobby(const uint64 SteamLobbyID)
{
	const SteamAPICall_t SteamJoinShadowLobbyAPICall = SteamMatchmaking()->JoinLobby(SteamLobbyID);
	OnShadowLobbyEnterCallResult.Set(SteamJoinShadowLobbyAPICall, this, &USteamLobbySubsystem::OnJoinShadowLobbyComplete);
}

void USteamLobbySubsystem::OnJoinShadowLobbyComplete(LobbyEnter_t* Data, bool bIOFailure)
{
	if(!LocalUserSubsystem || !LobbySubsystem) return;
	SetLobbyID(Data->m_ulSteamIDLobby); // Will be 0 if failed to join
	
	if(Data->m_EChatRoomEnterResponse == k_EChatRoomEnterResponseSuccess)
	{
		UE_LOG(LogSteamLobbySubsystem, Log, TEXT("Shadow-lobby joined."));

		// Join the EOS lobby if the user is not already in the EOS lobby.
		if(!LobbySubsystem->InLobby())
		{
			const char* EosLobbyID = SteamMatchmaking()->GetLobbyData(Data->m_ulSteamIDLobby, "EOSLobbyID");
			LobbySubsystem->JoinLobbyByID(EosLobbyID);
		}
	}
	else
		UE_LOG(LogSteamLobbySubsystem, Log, TEXT("Failed to join Shadow-lobby. m_EChatRoomEnterResponse: [%i]"), Data->m_EChatRoomEnterResponse);
}


// --------------------------------------------


/**
 * Called when the user tries to join a lobby from their friends list.
 */
void USteamLobbySubsystem::OnJoinShadowLobbyRequest(GameLobbyJoinRequested_t* Data)
{
	if(Data && Data->m_steamIDLobby.IsValid()) JoinShadowLobby(Data->m_steamIDLobby.ConvertToUint64());
}

/**
 * Called when the user tries to join a game from their friends list.
 */
void USteamLobbySubsystem::OnJoinRichPresenceRequest(GameRichPresenceJoinRequested_t* Data)
{
	UE_LOG(LogSteamLobbySubsystem, Log, TEXT("Joining shadow lobby through rich presence..."));
}


// --------------------------------------------


TArray<UOnlineUser*> USteamLobbySubsystem::GetMemberList() const
{
	TArray<UOnlineUser*> OutMembers;
	LobbyDetails.MemberList.GenerateValueArray(OutMembers);
	return OutMembers;
}

UOnlineUser* USteamLobbySubsystem::GetMember(const FString UserID)
{
	UOnlineUser** FoundUser = LobbyDetails.MemberList.Find(UserID);
	return FoundUser ? *FoundUser : nullptr;
}

void USteamLobbySubsystem::StoreMember(UOnlineUser* OnlineUser)
{
	if(!OnlineUser || OnlineUser->GetUserID().IsEmpty())
	{
		UE_LOG(LogSteamLobbySubsystem, Warning, TEXT("Invalid User provided."));
		return;
	}
	LobbyDetails.MemberList.Add(OnlineUser->GetUserID(), OnlineUser);
}

void USteamLobbySubsystem::StoreMembers(TArray<UOnlineUser*>& OnlineUserList)
{
	for (UOnlineUser* OnlineUser : OnlineUserList)
	{
		StoreMember(OnlineUser);
	}
}

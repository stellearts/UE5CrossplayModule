// Copyright © 2023 Melvin Brink

#include "Subsystems/Friends/FriendsSubsystem.h"
#include "Subsystems/User/Local/LocalUserSubsystem.h"
#include "EOSManager.h"
#include "Subsystems/Friends/SteamFriendsSubsystem.h"
#include "Subsystems/Lobby/SteamLobbySubsystem.h"


UFriendsSubsystem::UFriendsSubsystem() : EosManager(&FEosManager::Get())
{
}

void UFriendsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	const EOS_HPlatform PlatformHandle = EosManager->GetPlatformHandle();
	if(!PlatformHandle) return;
	FriendsHandle = EOS_Platform_GetFriendsInterface(PlatformHandle);

	SteamFriendsSubsystem = Collection.InitializeDependency<USteamFriendsSubsystem>();
	LocalUserSubsystem = Collection.InitializeDependency<ULocalUserSubsystem>();
}


// --------------------------------------------


TArray<FPlatformUser> UFriendsSubsystem::GetFriendList() const
{
	// TODO: Other platforms.
	if(LocalUserSubsystem->GetLocalUser()->GetPlatform() == EPlatform::Steam)
	{
		return SteamFriendsSubsystem->GetFriendList();
	}

	return TArray<FPlatformUser>();
}


// --------------------------------------------


void UFriendsSubsystem::InviteToLobby(const FPlatformUser& PlatformUser) const
{
	// If both from EOS Send an invite using EOS_Lobby_SendInvite using their product user id.
	// If both friends from same platform, send invite using that platform.

	// Check platform and get shadow-lobby id of that platform.
	const USteamLobbySubsystem* SteamLobbySubsystem = GetGameInstance()->GetSubsystem<USteamLobbySubsystem>();
	const FString ShadowLobbyID = SteamLobbySubsystem->GetLobbyID();
	SteamFriendsSubsystem->InviteToLobby(ShadowLobbyID, PlatformUser.UserID);
}


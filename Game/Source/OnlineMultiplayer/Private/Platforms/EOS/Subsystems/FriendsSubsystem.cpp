// Copyright © 2023 Melvin Brink

#include "Platforms/EOS/Subsystems/FriendsSubsystem.h"
#include "Platforms/EOS/Subsystems/LocalUserSubsystem.h"
#include "Platforms/EOS/EOSManager.h"
#include "Platforms/Steam/Subsystems/SteamFriendsSubsystem.h"


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


TArray<UPlatformUser*> UFriendsSubsystem::GetFriendList() const
{
	// TODO: Other platforms.
	if(LocalUserSubsystem->GetLocalUser()->GetPlatform() == EOS_EExternalAccountType::EOS_EAT_STEAM)
	{
		return SteamFriendsSubsystem->GetFriendList();
	}

	return TArray<UPlatformUser*>();
}


// --------------------------------------------


void UFriendsSubsystem::InviteToLobby(UPlatformUser* PlatformUser)
{
	// If both from EOS Send an invite using EOS_Lobby_SendInvite using their product user id.
	// If both friends from same platform, send invite using that platform.

	// Check platform and get shadow-lobby id of that platform.
	const FString ShadowLobbyID = LocalUserSubsystem->GetLocalUser()->GetShadowLobbyID();
	SteamFriendsSubsystem->InviteToLobby(ShadowLobbyID, PlatformUser->GetPlatformID());
}


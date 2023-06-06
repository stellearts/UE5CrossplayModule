// Copyright © 2023 Melvin Brink

#include "Platforms/EOS/Subsystems/FriendsSubsystem.h"
#include "Platforms/EOS/Subsystems/LocalUserSubsystem.h"
#include "Platforms/EOS/EOSManager.h"


void UFriendsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Make sure the local user state subsystem is initialized.
	LocalUserSubsystem = Collection.InitializeDependency<ULocalUserSubsystem>();

	EosManager = &FEosManager::Get();
	const EOS_HPlatform PlatformHandle = EosManager->GetPlatformHandle();
	if(!PlatformHandle) return;
	FriendsHandle = EOS_Platform_GetFriendsInterface(PlatformHandle);
}


// --------------------------------------------


void UFriendsSubsystem::InviteFriendToLobby()
{
	// TODO:
	// Send an invite using EOS_Lobby_SendInvite using their product user id.
	// If both friends from steam, also send invite using steam SDK to show it in overlay.
}

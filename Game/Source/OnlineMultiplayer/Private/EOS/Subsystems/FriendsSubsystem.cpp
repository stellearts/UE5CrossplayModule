// Copyright © 2023 Melvin Brink

#include "EOS/Subsystems/FriendsSubsystem.h"

#include "EOS/EOSManager.h"
#include "eos_friends.h"
#include "EOS/Subsystems/LocalUserStateSubsystem.h"


void UFriendsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Make sure the local user state subsystem is initialized.
	LocalUserState = Collection.InitializeDependency<ULocalUserStateSubsystem>();

	EosManager = &FEosManager::Get();
	const EOS_HPlatform PlatformHandle = EosManager->GetPlatformHandle();
	if(!PlatformHandle) return;
	FriendsHandle = EOS_Platform_GetFriendsInterface(PlatformHandle);
}


// --------------------------------------------


void UFriendsSubsystem::InviteFriend()
{
	// TODO:
	// Send an invite using EOS_Lobby_SendInvite using their product user id.
	// If both friends from steam, also send invite using steam SDK to show it in overlay.
}

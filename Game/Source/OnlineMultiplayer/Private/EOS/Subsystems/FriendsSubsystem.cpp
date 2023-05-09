// Copyright © 2023 Melvin Brink

#include "EOS/Subsystems/FriendsSubsystem.h"

#include "EOS/EOSManager.h"
#include "eos_friends.h"



void UFriendsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	EosManager = &FEosManager::Get();
	const EOS_HPlatform PlatformHandle = EosManager->GetPlatformHandle();
	if(!PlatformHandle)
	{
		UE_LOG(LogFriendsSubsystem, Error, TEXT("Platform-Handle is null"));
		return;
	}
	
	FriendsHandle = EOS_Platform_GetFriendsInterface(PlatformHandle);
}


// --------------------------------------------


void UFriendsSubsystem::InviteFriend()
{
	// TODO:
	// Send an invite using EOS_Lobby_SendInvite using their product user id.
	// If both friends from steam, also send invite using steam SDK to show it in overlay.
}

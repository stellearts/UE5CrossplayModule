// Copyright © 2023 Melvin Brink

#include "Platforms/EOS/Subsystems/FriendsSubsystem.h"
#include "Platforms/EOS/Subsystems/LocalUserSubsystem.h"
#include "Platforms/EOS/EOSManager.h"
#include "Platforms/Steam/Subsystems/SteamFriendsSubsystem.h"


void UFriendsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	EosManager = &FEosManager::Get();
	const EOS_HPlatform PlatformHandle = EosManager->GetPlatformHandle();
	if(!PlatformHandle) return;
	FriendsHandle = EOS_Platform_GetFriendsInterface(PlatformHandle);

	SteamFriendsSubsystem = Collection.InitializeDependency<USteamFriendsSubsystem>();
	LocalUserSubsystem = Collection.InitializeDependency<ULocalUserSubsystem>();
}

TArray<UPlatformUser*> UFriendsSubsystem::GetFriendList() const
{
	// TODO: Other platforms.
	if(LocalUserSubsystem->GetLocalUser()->GetPlatform() == EOS_EExternalAccountType::EOS_EAT_STEAM)
	{
		return SteamFriendsSubsystem->GetFriendList();
	}

	return TArray<UPlatformUser*>();
}

void UFriendsSubsystem::InviteToLobby()
{
}


// --------------------------------------------


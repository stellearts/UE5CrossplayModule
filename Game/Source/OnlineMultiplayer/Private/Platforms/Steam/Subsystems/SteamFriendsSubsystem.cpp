// Copyright © 2023 Melvin Brink

#include "Platforms/Steam/Subsystems/SteamFriendsSubsystem.h"
#include "Platforms/EOS/Subsystems/LocalUserSubsystem.h"
#include <vector>

#pragma warning(push)
#pragma warning(disable: 4996)
#pragma warning(disable: 4265)
#include "isteamfriends.h"
#include "isteamutils.h"
#include "steam_api.h"
#pragma warning(pop)



void USteamFriendsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}


// --------------------------------------------


/**
 * Get the Steam Friend list.
 *
 * Will cache the friends in a map. Friend updates will happen in OnPersonaStateChange.
 */
TArray<UPlatformUser*> USteamFriendsSubsystem::GetFriendList()
{
	TArray<UPlatformUser*> OutFriendList;
	if(FriendList.Num() > 0){
		FriendList.GenerateValueArray(OutFriendList);
		return OutFriendList;
	}

	// Loop through each friend, get their information and add to the FriendList.
	const int FriendCount = SteamFriends()->GetFriendCount( k_EFriendFlagImmediate );
	TMap<FString, UPlatformUser*> SteamFriendsMap;
	for(int i=0; i<FriendCount; ++i )
	{
		UPlatformUser* SteamUser = NewObject<UPlatformUser>();
		
		// Get the User ID
		const CSteamID SteamUserID = SteamFriends()->GetFriendByIndex( i, k_EFriendFlagImmediate );
		SteamUser->SetPlatformID(SteamUserID.ConvertToUint64());

		// Get the Username
		FString Username = SteamFriends()->GetFriendPersonaName(SteamUserID);
		SteamUser->SetUsername(Username);

		// Get the Avatar Handle and use it to fetch the image
		const int AvatarHandle = SteamFriends()->GetMediumFriendAvatar(SteamUserID);
		if(AvatarHandle != 0) SteamUser->SetAvatar(CreateTextureFromAvatar(AvatarHandle));
		
		FriendList.Add(SteamUser->GetPlatformID(), SteamUser);
	}
	
	FriendList.GenerateValueArray(OutFriendList);
	return OutFriendList;
}


/**
 * Called whenever a user's state has been changed, and will update the cached friends data to reflect that.
 *
 * Will only update friends of the user.
 *
 * Bind to corresponding delegates to receive these changes. // TODO: this line.
 */
void USteamFriendsSubsystem::OnPersonaStateChange(PersonaStateChange_t* Data)
{
	const CSteamID SteamUserID(Data->m_ulSteamID);
	UPlatformUser** SteamUser = FriendList.Find(FString::FromInt(SteamUserID.ConvertToUint64()));
	if(SteamUser) return;

	// Avatar changed.
	if(Data->m_nChangeFlags & k_EPersonaChangeAvatar)
	{
		// Friend's Avatar has changed, update the image.
		const int AvatarHandle = SteamFriends()->GetMediumFriendAvatar(SteamUserID);
		if(AvatarHandle != 0) (*SteamUser)->SetAvatar(CreateTextureFromAvatar(AvatarHandle)); // TODO: Crashed here with AvatarHandle=2 and SteamUser=NULL.
	}
}


// TODO: Placeholder avatar for if there isn't any.
UTexture2D* USteamFriendsSubsystem::CreateTextureFromAvatar(const int AvatarHandle) const
{
	uint32 Width, Height;
	if (!SteamUtils()->GetImageSize(AvatarHandle, &Width, &Height)) return nullptr;
	
	std::vector<uint8> Buffer(Width * Height * 4); // 4 for RGBA
	if (!SteamUtils()->GetImageRGBA(AvatarHandle, Buffer.data(), Buffer.size())) return nullptr;
	
	UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);

	#if WITH_EDITORONLY_DATA
	Texture->MipGenSettings = TMGS_NoMipmaps;
	#endif

	uint8* MipData = (uint8*)Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(MipData, Buffer.data(), Width * Height * 4); // 4 for RGBA
	Texture->GetPlatformData()->Mips[0].BulkData.Unlock();
	Texture->UpdateResource();

	return Texture;
}


// --------------------------------------------


void USteamFriendsSubsystem::InviteToLobby()
{
	// TODO:
	// Send an invite using EOS_Lobby_SendInvite using their product user id.
	// If both friends from steam, also send invite using steam SDK to show it in overlay.
}

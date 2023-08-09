// Copyright © 2023 Melvin Brink

#include "Subsystems/Friends/SteamFriendsSubsystem.h"
#include "Subsystems/User/Local/LocalUserSubsystem.h"
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
TArray<FPlatformUser> USteamFriendsSubsystem::GetFriendList()
{
	// TODO: Separate into GetFriendList and LoadFriendList, then call LoadFriendList (or FetchFriendList) when initializing all subsystems so that it will be ready when calling GetFriendList in blueprint or whatever.
	// Return the list when it has already been filled
	TArray<FPlatformUser> OutFriendList;
	if(FriendList.Num() > 0){
		FriendList.GenerateValueArray(OutFriendList);
		return OutFriendList;
	}

	// Loop through each friend, get their information and add to the FriendList.
	const int FriendCount = SteamFriends()->GetFriendCount( k_EFriendFlagImmediate );
	TMap<FString, FPlatformUser> SteamFriendsMap;
	for(int i=0; i<FriendCount; ++i )
	{
		FPlatformUser SteamUser;
		
		// User-ID
		const CSteamID SteamUserID = SteamFriends()->GetFriendByIndex( i, k_EFriendFlagImmediate );
		SteamUser.UserID = FString::Printf(TEXT("%llu"), SteamUserID.ConvertToUint64());

		// Username
		SteamUser.Username = FString(SteamFriends()->GetFriendPersonaName(SteamUserID));

		// Avatar
		const int AvatarHandle = SteamFriends()->GetMediumFriendAvatar(SteamUserID);
		if(AvatarHandle != 0) SteamUser.Avatar = CreateTextureFromAvatar(AvatarHandle);
		
		FriendList.Add(SteamUser.UserID, SteamUser);
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
	FPlatformUser* SteamUser = FriendList.Find(FString::Printf(TEXT("%llu"), SteamUserID.ConvertToUint64()));
	if(!SteamUser) return;

	// Avatar changed.
	if(Data->m_nChangeFlags & k_EPersonaChangeAvatar)
	{
		// Friend's Avatar has changed, update the image.
		const int AvatarHandle = SteamFriends()->GetMediumFriendAvatar(SteamUserID);
		if(AvatarHandle != 0) (*SteamUser).Avatar = CreateTextureFromAvatar(AvatarHandle); // TODO: Crashed here with AvatarHandle=2 and SteamUser=NULL.
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


void USteamFriendsSubsystem::InviteToLobby(const FString& LobbyID, const FString& UserID)
{
	const uint64 SteamLobbyID = FCString::Strtoui64(*LobbyID, nullptr, 10);
	const uint64 SteamUserID = FCString::Strtoui64(*UserID, nullptr, 10);
	
	if (!SteamMatchmaking()->InviteUserToLobby(SteamLobbyID, SteamUserID))
	{
		UE_LOG(LogSteamFriendsSubsystem, Warning, TEXT("Failed to send lobby invite."));
	}
}

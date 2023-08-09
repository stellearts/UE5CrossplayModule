// Copyright © 2023 Melvin Brink

#include "Subsystems/User/Online/SteamOnlineUserSubsystem.h"
#include "Utils/UserUtils.h"
#include <vector>

#pragma warning(push)
#pragma warning(disable: 4996)
#pragma warning(disable: 4265)
#include "steam_api.h"
#include "isteamuser.h"
#include "steamnetworkingtypes.h"
#pragma warning(pop)



void USteamOnlineUserSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}


// --------------------------------------------


void USteamOnlineUserSubsystem::FetchAvatar(const uint64 UserID, const TFunction<void(UTexture2D*)> &Callback)
{
	const CSteamID SteamUserID(UserID);
	
	// False when the image is not ready yet. Add callback to the map of callbacks which will be called when ready
	if (SteamFriends()->RequestUserInformation(SteamUserID, false)) // TODO: Will the OnPersonaStateChange always be triggered when calling this?
	{
		FetchAvatarCallbacks.Add(UserID, Callback);
		return;
	}

	// Image should be ready
	if (const int ImageData = SteamFriends()->GetLargeFriendAvatar(SteamUserID); ImageData) 
	{
		Callback(ProcessAvatar(ImageData));
	}
	else if(ImageData == 0)
	{
		// User has no avatar set.
		Callback(nullptr);
	}
	else if(ImageData == -1)
	{
		// Avatar is still not ready yet. (Should not reach)
		FetchAvatarCallbacks.Add(UserID, Callback);
	}
	
}

UTexture2D* USteamOnlineUserSubsystem::ProcessAvatar(const int& ImageData)
{
	uint32 ImageWidth, ImageHeight;
	if (SteamUtils()->GetImageSize(ImageData, &ImageWidth, &ImageHeight)) 
	{
		// Create buffer large enough to hold the image data
		std::vector<uint8> Buffer(ImageWidth * ImageHeight * 4); // 4 bytes per pixel for RGBA

		// Get the actual image data
		if (SteamUtils()->GetImageRGBA(ImageData, Buffer.data(), Buffer.size())) 
		{
			// Avatar image data is now in the buffer
			// You can use this data to create a texture, save it to a file, etc.
			return FUserUtils::ImageBufferToTexture2D(Buffer, ImageWidth, ImageHeight); // TODO: Return struct FFetchAvatarResult? or bool bSuccess?
		}
		
		return nullptr; // Image does not exist.
	}
	
	return nullptr; // Image does not exist.
}


void USteamOnlineUserSubsystem::OnPersonaStateChange(PersonaStateChange_t *Data)
{
	if(!Data) return;
	const CSteamID SteamUserID(Data->m_ulSteamID);
	const uint64 UserID = SteamUserID.ConvertToUint64();

	// If avatar data has changed
	if (Data->m_nChangeFlags & k_EPersonaChangeAvatar)
	{

		// If there is a callback in this map, then a function is waiting for it to be called.
		if(FetchAvatarCallbacks.Num())
		{
			// If there is a callback for this ID, then it should be called since the image is now ready
			const TFunction<void(UTexture2D*)>* FetchAvatarCallback = FetchAvatarCallbacks.Find(UserID);
			if(!FetchAvatarCallback) return;
			
			FetchAvatarCallbacks.Remove(UserID);
			const int ImageData = SteamFriends()->GetLargeFriendAvatar(SteamUserID); // Image should be ready
			(*FetchAvatarCallback)(ProcessAvatar(ImageData));
		}

		// TODO: Listen for user changes and broadcast a delegate so that other modules can react to the changes.
		
	}
}


void USteamOnlineUserSubsystem::OnAvatarImageLoaded(AvatarImageLoaded_t *pParam)
{
	if(!pParam || !pParam->m_steamID.IsValid()) return;
	const CSteamID SteamUserID(pParam->m_steamID);
	const uint64 UserID = SteamUserID.ConvertToUint64();
	
	const TFunction<void(UTexture2D*)>* FetchAvatarCallback = FetchAvatarCallbacks.Find(UserID);
	if(!FetchAvatarCallback) return;
	
	FetchAvatarCallbacks.Remove(UserID);
	const int ImageData = SteamFriends()->GetLargeFriendAvatar(SteamUserID); // Image should be ready
	(*FetchAvatarCallback)(ProcessAvatar(ImageData));
}
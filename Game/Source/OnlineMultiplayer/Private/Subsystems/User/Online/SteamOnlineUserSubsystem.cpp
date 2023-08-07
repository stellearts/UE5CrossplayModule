// Copyright © 2023 Melvin Brink

#include "Subsystems/User/Online/SteamOnlineUserSubsystem.h"
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
		if(!FetchAvatarCallbacks.Find(UserID)) FetchAvatarCallbacks.Add(UserID, Callback); // Check if not already in the map
		return;
	}

	// Image is ready.
	Callback(ProcessAvatar(SteamUserID));
}

UTexture2D* USteamOnlineUserSubsystem::ProcessAvatar(const CSteamID& SteamUserID)
{
	// Listen to the callback when the image is not yet ready
	if (const int ImageData = SteamFriends()->GetLargeFriendAvatar(SteamUserID); ImageData != -1) 
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
				return BufferToTexture2D(Buffer, ImageWidth, ImageHeight); // TODO: Return struct FFetchAvatarResult? or bool bSuccess?
			}
		}else
		{
			// TODO: Image does not exist here.
			return nullptr; // test line
		}
	}
	
	return nullptr; // test line
}

UTexture2D* USteamOnlineUserSubsystem::BufferToTexture2D(std::vector<uint8>& Buffer, uint32 Width, uint32 Height)
{
	if (Buffer.size() != Width * Height * 4)
	{
		// Buffer size is not as expected, handle error
		return nullptr;
	}

	// Create a new texture
	UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PF_R8G8B8A8);
	if (!Texture)
	{
		// Failed to create texture, handle error
		return nullptr;
	}

	// Copy the pixel data to the texture
	FTexturePlatformData* TexturePlatformData = Texture->GetPlatformData();
	void* TextureData = TexturePlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(TextureData, Buffer.data(), Buffer.size());
	TexturePlatformData->Mips[0].BulkData.Unlock();

	// Update the texture resource
	Texture->UpdateResource();

	return Texture;
}


void USteamOnlineUserSubsystem::OnPersonaStateChange(PersonaStateChange_t* Data)
{
	const CSteamID SteamUserID(Data->m_ulSteamID);
	const uint64 UserID = SteamUserID.ConvertToUint64();

	// If avatar data changed, process it again.
	if (Data->m_nChangeFlags & k_EPersonaChangeAvatar)
	{
		// If there is a callback in the FetchAvatarCallbacks-map for this ID, then it should be called since the image is now ready
		if(const TFunction<void(UTexture2D*)>* FetchAvatarCallback = FetchAvatarCallbacks.Find(UserID); FetchAvatarCallback)
		{
			FetchAvatarCallbacks.Remove(UserID);
			FetchAvatar(UserID, *FetchAvatarCallback);
		}
		else
		{
			// TODO: What if a friend changes their avatar? and will it reach this line?	
		}
	}
}
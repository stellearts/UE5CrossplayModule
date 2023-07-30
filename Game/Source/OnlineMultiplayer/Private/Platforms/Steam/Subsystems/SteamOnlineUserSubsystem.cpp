// Copyright © 2023 Melvin Brink

#include "Platforms/Steam/Subsystems/SteamOnlineUserSubsystem.h"
#include <string>
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


void USteamOnlineUserSubsystem::FetchAvatar(const FString& UserIDString, const TFunction<void>& Callback)
{
	CHECK_STEAM
	const uint64 UserID = FCString::Strtoui64(*UserIDString, nullptr, 10);
	if(!UserID)
	{
		UE_LOG(LogSteamOnlineUserSubsystem, Warning, TEXT("Invalid User-ID given in USteamOnlineUserSubsystem::FetchAvatar"));
		return;
	}
	
	const CSteamID SteamUserID(UserID);
	
	// Return if the image is already being requested. Callback will be called when the image is ready.
	if (SteamFriends()->RequestUserInformation(SteamUserID, false)) return;

	// Image is ready.
	ProcessAvatar(SteamUserID);
}

void USteamOnlineUserSubsystem::ProcessAvatar(const CSteamID& SteamUserID)
{
	// Listen to the callback when the image is not yet ready
	if (const int ImageData = SteamFriends()->GetSmallFriendAvatar(SteamUserID); ImageData != -1) 
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
			}
		}
	}
}

void USteamOnlineUserSubsystem::OnPersonaStateChange(PersonaStateChange_t* Data)
{
	CHECK_STEAM
	const CSteamID UserID(Data->m_ulSteamID);

	// If avatar data changed, process it again.
	if (Data->m_nChangeFlags & k_EPersonaChangeAvatar)
	{
		
	}
}

#include "Utils/UserUtils.h"



/*
 * Uses the given image-buffer to create a Texture2D that can be used by Unreal.
 */
UTexture2D* FUserUtils::ImageBufferToTexture2D(const std::vector<uint8>& Buffer, const uint32 Width, const uint32 Height)
{
	if (Buffer.size() != Width * Height * 4)
	{
		// Buffer size is not as expected. Todo handle error
		return nullptr;
	}

	// Create a new texture
	UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PF_R8G8B8A8);
	if (!Texture)
	{
		// Failed to create texture. Todo handle error
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

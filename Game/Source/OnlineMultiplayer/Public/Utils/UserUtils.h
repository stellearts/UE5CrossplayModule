#pragma once

#include "CoreMinimal.h"
#include <vector>

/**
 * Class providing helper methods for users.
 */
class ONLINEMULTIPLAYER_API FUserUtils
{
public:
	static UTexture2D* ImageBufferToTexture2D(const std::vector<uint8>& Buffer, const uint32 Width, const uint32 Height);
};
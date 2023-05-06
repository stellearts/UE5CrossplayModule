// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

#pragma warning(push)
#pragma warning(disable: 4996)
#include "steam_api.h"
#pragma warning(pop)



/**
 * Test module for the Steam SDK.
 */
class FOnlineMultiplayer : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
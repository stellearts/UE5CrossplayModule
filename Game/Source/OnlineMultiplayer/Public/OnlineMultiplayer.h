// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

#pragma warning(push)
#pragma warning(disable: 4996)
#pragma warning(pop)

DECLARE_LOG_CATEGORY_EXTERN(LogMultiplayerModule, Log, All);
inline DEFINE_LOG_CATEGORY(LogMultiplayerModule);



/**
 * Cross-play module which initializes the platform specific singleton's and has some helper functionality.
 */
class FOnlineMultiplayer : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
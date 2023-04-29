#include "SteamModule.h"
// #include "Steam/steam_api.h"

#define LOCTEXT_NAMESPACE "FSteamModuleModule"



void FSteamModuleModule::StartupModule()
{
	UE_LOG(LogTemp, Warning, TEXT("FSteamModuleModule::StartupModule"));
	// Initialize the Steam SDK.
	// if (SteamAPI_Init())
	// {
	// 	UE_LOG(LogTemp, Log, TEXT("Steam SDK initialized successfully."));
	// }
	// else
	// {
	// 	UE_LOG(LogTemp, Error, TEXT("Failed to initialize Steam SDK."));
	// }
}

void FSteamModuleModule::ShutdownModule()
{
    // SteamAPI_Shutdown();
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FSteamModuleModule, SteamModule)
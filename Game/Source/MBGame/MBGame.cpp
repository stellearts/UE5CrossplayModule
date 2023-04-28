// Copyright Epic Games, Inc. All Rights Reserved.

#include "MBGame.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_PRIMARY_GAME_MODULE( FDefaultGameModuleImpl, MBGame, "MBGame" );



/*void FMBGameModule::StartupModule()
{
	UE_LOG(LogMBGameModule, Warning, TEXT("MBGameModule: Log Started"));
	
	// Initialize the EOS-SDK.
	InitializeEosSdk();

	// Initialize the platform.
	InitializeEosPlatform();
}

void FMBGameModule::ShutdownModule()
{

}

/**
 * Initializes the EOS-SDK.
 * 
 * Must be called first in order to use it.
 #1#
void FMBGameModule::InitializeEosSdk()
{
	EOS_InitializeOptions InitOptions;
	InitOptions.ApiVersion = EOS_INITIALIZE_API_LATEST;
	InitOptions.AllocateMemoryFunction = nullptr;
	InitOptions.ReallocateMemoryFunction = nullptr;
	InitOptions.ReleaseMemoryFunction = nullptr;
	InitOptions.ProductName = "MBGame";
	InitOptions.ProductVersion = "0.0.1";
	InitOptions.Reserved = nullptr;
	InitOptions.SystemInitializeOptions = nullptr;
	InitOptions.OverrideThreadAffinity = nullptr;

	const EOS_EResult InitResult = EOS_Initialize(&InitOptions);
	if (InitResult != EOS_EResult::EOS_Success)
	{
		UE_LOG(LogMBGameModule, Error, TEXT("Failed to initialize EOS SDK: %s"), *FString(EOS_EResult_ToString(InitResult)));
		return;
	}
	UE_LOG(LogMBGameModule, Log, TEXT("EOS SDK initialized successfully"));
}

/**
 * Initializes the EOS-Platform.
 * 
 * Will provide access to all other EOS-SDK interfaces.
 *
 * Sets 'EosPlatformHandle' to a valid handle if successful.
 #1#
void FMBGameModule::InitializeEosPlatform()
{
	// Create the temporary container.
	EOS_Platform_Options PlatformOptions = {};
	if(CreateIntegratedPlatform(PlatformOptions) != EOS_EResult::EOS_Success)
	{
		UE_LOG(LogMBGameModule, Error, TEXT("Failed to create Integrated-Platform"));
		return;
	}

	// Set the options.
	PlatformOptions.ApiVersion = EOS_PLATFORM_OPTIONS_API_LATEST;
	PlatformOptions.ProductId = "cf6e819c86fc45548c72029b93e59853";
	PlatformOptions.SandboxId = "3bd37bf86718447bbb4f5d7473212ec5";
	PlatformOptions.DeploymentId = "7c5d0e6b46f2429aab485c838008d458";
	PlatformOptions.ClientCredentials.ClientId = "xyza7891cHQIAxVpn4iwHSPLv4JGOZp6";
	PlatformOptions.ClientCredentials.ClientSecret = "RnQd8t2c/OR8qKpQ449re2zln9lheKHL5G9i9x/A5H4";
	PlatformOptions.bIsServer = EOS_FALSE;
	PlatformOptions.EncryptionKey = nullptr;
	PlatformOptions.OverrideCountryCode = nullptr;
	PlatformOptions.OverrideLocaleCode = nullptr;
	PlatformOptions.CacheDirectory = nullptr;
	PlatformOptions.TickBudgetInMilliseconds = 0;
	PlatformOptions.RTCOptions = nullptr;
	PlatformOptions.IntegratedPlatformOptionsContainerHandle = nullptr;
	// PlatformOptions.Flags = EOS_PF_LOADING_IN_EDITOR;

	// Initialize the platform handle.
	EosPlatformHandle = EOS_Platform_Create(&PlatformOptions);
	if (!EosPlatformHandle)
	{
		// Handle error
		UE_LOG(LogMBGameModule, Error, TEXT("Failed to initialize EOS Platform"));
		return;
	}
	UE_LOG(LogMBGameModule, Log, TEXT("EOS Platform initialized successfully"));

	// Release the temporary container.
	FreeIntegratedPlatform(PlatformOptions);
}

/**
 * Creates a temporary container for the platform-specific options.
 * 
 * Should be released with 'FreeIntegratedPlatform' after use.
 *
 * @param Options The options to configure.
 #1#
EOS_EResult FMBGameModule::CreateIntegratedPlatform(EOS_Platform_Options& PlatformOptions)
{
	// Create the generic container.
	constexpr EOS_IntegratedPlatform_CreateIntegratedPlatformOptionsContainerOptions CreateOptions =
	{
		EOS_INTEGRATEDPLATFORM_CREATEINTEGRATEDPLATFORMOPTIONSCONTAINER_API_LATEST
	};

	const EOS_EResult Result = EOS_IntegratedPlatform_CreateIntegratedPlatformOptionsContainer(
		&CreateOptions, &PlatformOptions.IntegratedPlatformOptionsContainerHandle);
	if (Result != EOS_EResult::EOS_Success)
	{
		return Result;
	}

	// Configure platform-specific options.
	const EOS_IntegratedPlatform_Steam_Options PlatformSpecificOptions =
	{
		EOS_INTEGRATEDPLATFORM_STEAM_OPTIONS_API_LATEST,
		nullptr,
		1,
		48
	};

	// Add the configuration to the SDK initialization options.
	const EOS_IntegratedPlatform_Options Options =
	{
		EOS_INTEGRATEDPLATFORM_OPTIONS_API_LATEST,
		EOS_IPT_Steam,
		EOS_EIntegratedPlatformManagementFlags::EOS_IPMF_LibraryManagedByApplication |
		EOS_EIntegratedPlatformManagementFlags::EOS_IPMF_DisableSDKManagedSessions,
		&PlatformSpecificOptions
	};

	const EOS_IntegratedPlatformOptionsContainer_AddOptions AddOptions =
	{
		EOS_INTEGRATEDPLATFORMOPTIONSCONTAINER_ADD_API_LATEST,
		&Options
	};

	return EOS_IntegratedPlatformOptionsContainer_Add(PlatformOptions.IntegratedPlatformOptionsContainerHandle,
	                                                  &AddOptions);
}

/**
 * Releases the temporary container created by 'CreateIntegratedPlatform'.
 *
 * @param Options The options that hold the container
 #1#
void FMBGameModule::FreeIntegratedPlatform(EOS_Platform_Options& PlatformOptions)
{
	// Free the container after SDK initialization.
	if (PlatformOptions.IntegratedPlatformOptionsContainerHandle)
	{
		EOS_IntegratedPlatformOptionsContainer_Release(PlatformOptions.IntegratedPlatformOptionsContainerHandle);
		PlatformOptions.IntegratedPlatformOptionsContainerHandle = nullptr;
	}
}*/
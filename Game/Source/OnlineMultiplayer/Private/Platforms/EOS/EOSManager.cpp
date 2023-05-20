// Copyright © 2023 Melvin Brink

#include "Platforms/EOS/EOSManager.h"
#include "Platforms/Steam/SteamManager.h"
#include <string>

#include "eos_sdk.h"
#include "eos_logging.h"
#include "eos_common.h"
#include "eos_integratedplatform.h"



/**
 * Receives useful logs from the EOS SDK.
 */
static void EOS_CALL OnEosLogMessage(const EOS_LogMessage* Message)
{
	// Temporary solution.
	if (Message)
	{
		switch (Message->Level)
		{
		case EOS_ELogLevel::EOS_LOG_Fatal:
			UE_LOG(LogEOSSubsystem, Fatal, TEXT("EOS [%hs]: %hs"), Message->Category, Message->Message);
			break;
		case EOS_ELogLevel::EOS_LOG_Error:
			UE_LOG(LogEOSSubsystem, Error, TEXT("EOS [%hs]: %hs"), Message->Category, Message->Message);
			break;
		case EOS_ELogLevel::EOS_LOG_Warning:
			UE_LOG(LogEOSSubsystem, Warning, TEXT("EOS [%hs]: %hs"), Message->Category, Message->Message);
			break;
		case EOS_ELogLevel::EOS_LOG_Info:
			UE_LOG(LogEOSSubsystem, Log, TEXT("EOS [%hs]: %hs"), Message->Category, Message->Message);
			break;
		case EOS_ELogLevel::EOS_LOG_Verbose:
		case EOS_ELogLevel::EOS_LOG_VeryVerbose:
			UE_LOG(LogEOSSubsystem, Verbose, TEXT("EOS [%hs]: %hs"), Message->Category, Message->Message);
			break;
		case EOS_ELogLevel::EOS_LOG_Off:
		default:
			break;
		}
	}
}

FEosManager::~FEosManager()
{
	
}

void FEosManager::Tick(float DeltaTime)
{
	if (PlatformHandle) EOS_Platform_Tick(PlatformHandle); // If-statement maybe redundant because of IsTickable().
}

bool FEosManager::IsTickable() const
{
	return PlatformHandle != nullptr;
}

TStatId FEosManager::GetStatId() const
{
	// Return an empty TStatId (used for profiling)
	return TStatId();
}

FEosManager::FEosManager()
{
}

FEosManager& FEosManager::Get()
{
	static FEosManager Instance;
	return Instance;
}


// --------------------------------


void FEosManager::Initialize()
{
	// Register the logging callback.
	EOS_Logging_SetCallback(&OnEosLogMessage);
	EOS_Logging_SetLogLevel(EOS_ELogCategory::EOS_LC_ALL_CATEGORIES, EOS_ELogLevel::EOS_LOG_Verbose);

	// Initialize the SDK and the platform. Order is important here.
	InitializeSdk();
	InitializePlatform();


	// TODO: Steam should not run at all if playing on different platform, find a way to disable it and run this conditionally.
	// Get the SteamManager and set necessary callbacks.
	SteamManager = &FSteamManager::Get();
	SteamManager->OnSessionTicketReady.AddStatic(&FEosManager::OnSteamSessionTicketResponse);
	SteamManager->OnEncryptedAppTicketReady.AddStatic(&FEosManager::OnSteamEncryptedAppTicketResponse);

	// Start requesting the Steam Session-Ticket so that we don't have to wait for it later.
	// SteamManager->RequestSessionTicket();
}

/**
 * Initializes the EOS-SDK.
 */
void FEosManager::InitializeSdk()
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
		UE_LOG(LogEOSSubsystem, Error, TEXT("Failed to initialize EOS SDK: %s"), *FString(EOS_EResult_ToString(InitResult)));
		return;
	}
	UE_LOG(LogEOSSubsystem, Log, TEXT("EOS SDK initialized successfully"));
}

/**
 * Initializes the EOS-Platform.
 * 
 * Will provide access to all other EOS-SDK interfaces.
 *
 * Sets 'PlatformHandle' if successful.
 */
void FEosManager::InitializePlatform()
{
	// Create the temporary container.
	EOS_Platform_Options PlatformOptions = {};
	if(CreateIntegratedPlatform(PlatformOptions) != EOS_EResult::EOS_Success)
	{
		UE_LOG(LogEOSSubsystem, Error, TEXT("Failed to create Integrated-Platform"));
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
	PlatformHandle = EOS_Platform_Create(&PlatformOptions);
	if (!PlatformHandle)
	{
		// Handle error
		UE_LOG(LogEOSSubsystem, Error, TEXT("Failed to initialize EOS Platform"));
		return;
	}
	UE_LOG(LogEOSSubsystem, Log, TEXT("EOS Platform initialized successfully"));

	// Release the temporary container.
	FreeIntegratedPlatform(PlatformOptions);
}

/**
 * Creates a temporary container for the platform-specific options.
 * 
 * Should be released with 'FreeIntegratedPlatform' after use.
 *
 * @param EOS_Platform_Options The options to configure.
 */
EOS_EResult FEosManager::CreateIntegratedPlatform(EOS_Platform_Options& PlatformOptions)
{
	// TODO: Add support for other platforms. Currently only Steam is supported.
	
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
 * @param EOS_Platform_Options The options that hold the container
 */
void FEosManager::FreeIntegratedPlatform(EOS_Platform_Options& PlatformOptions)
{
	if (PlatformOptions.IntegratedPlatformOptionsContainerHandle)
	{
		EOS_IntegratedPlatformOptionsContainer_Release(PlatformOptions.IntegratedPlatformOptionsContainerHandle);
		PlatformOptions.IntegratedPlatformOptionsContainerHandle = nullptr;
	}
}


// --------------------------------


/**
 * Requests an encrypted app ticket from Steam.
 *
 * @param TicketReadyCallback The callback to call when the ticket is ready.
 */
void FEosManager::RequestSteamEncryptedAppTicket(const TFunction<void(std::string Ticket)> TicketReadyCallback)
{
	SteamEncryptedAppTicketCallback = TicketReadyCallback;
	SteamManager->RequestEncryptedAppTicket();
}

/**
 * Called when the Steam encrypted app ticket is ready.
 *
 * Converts the ticket to a string and broadcasts the delegate.
 */
void FEosManager::OnSteamEncryptedAppTicketResponse(const TArray<uint8> Ticket)
{
	if (Ticket.Num() > 0)
	{
		char Buffer[1024] = "";
		uint32_t Len = 1024;
		if (const EOS_EResult Result = EOS_ByteArray_ToString(Ticket.GetData(), Ticket.Num(), Buffer, &Len); Result != EOS_EResult::EOS_Success)
		{
			UE_LOG(LogEOSSubsystem, Error, TEXT("Failed to convert encrypted app ticket to string"));
		}
	
		Get().SteamEncryptedAppTicketCallback(Buffer);
	}
	else
	{
		UE_LOG(LogEOSSubsystem, Error, TEXT("Failed to get Auth Session Ticket from Steam"));
		Get().SteamEncryptedAppTicketCallback("");
	}

	// Clear callback
	Get().SteamEncryptedAppTicketCallback = TFunction<void(std::string TicketString)>();
}

/**
 * Requests a session ticket from Steam.
 *
 * @param TicketReadyCallback The callback to call when the ticket is ready.
 */
void FEosManager::RequestSteamSessionTicket(const TFunction<void(std::string TicketString)> TicketReadyCallback)
{
	SteamSessionTicketCallback = TicketReadyCallback;
	SteamManager->RequestSessionTicket();
}

/**
 * Called when the Steam session ticket is ready.
 *
 * Converts the ticket to a string and broadcasts the delegate.
 */
void FEosManager::OnSteamSessionTicketResponse(const TArray<uint8> Ticket)
{
	if (Ticket.Num() > 0)
	{
		char Buffer[1024] = "";
		uint32_t Len = 1024;
		if (const EOS_EResult Result = EOS_ByteArray_ToString(Ticket.GetData(), Ticket.Num(), Buffer, &Len); Result != EOS_EResult::EOS_Success)
		{
			UE_LOG(LogEOSSubsystem, Error, TEXT("Failed to convert encrypted app ticket to string"));
		}
	
		Get().SteamSessionTicketCallback(Buffer);
	}
	else
	{
		UE_LOG(LogEOSSubsystem, Error, TEXT("Failed to get Auth Session Ticket from Steam"));
		Get().SteamSessionTicketCallback("");
	}

	// Clear callback
	Get().SteamSessionTicketCallback = TFunction<void(std::string TicketString)>();
}
// Copyright © 2023 Melvin Brink

#include "EOS/Subsystems/SessionSubsystem.h"

#include "EOS/EOSManager.h"
#include "eos_sessions.h"



void USessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	EosManager = &FEosManager::Get();
	LocalUserState = EosManager->GetLocalUserState();
	
	const EOS_HPlatform PlatformHandle = EosManager->GetPlatformHandle();
	if(!PlatformHandle) return;
	SessionsHandle = EOS_Platform_GetSessionsInterface(PlatformHandle);
}


// --------------------------------------------


void USessionSubsystem::CreateSession()
{
	EOS_Sessions_CreateSessionModificationOptions CreateSessionOptions = {};
	CreateSessionOptions.ApiVersion = EOS_SESSIONS_CREATESESSIONMODIFICATION_API_LATEST;
	CreateSessionOptions.SessionName = "MySession";
	CreateSessionOptions.BucketId = "GameMode:Region:MapName";
	CreateSessionOptions.MaxPlayers = 8;
	CreateSessionOptions.LocalUserId = LocalUserState->GetProductUserId();
	CreateSessionOptions.bPresenceEnabled = true;
	CreateSessionOptions.bSanctionsEnabled = false;
	
	EOS_HSessionModification SessionModification;
	const EOS_EResult Result = EOS_Sessions_CreateSessionModification(SessionsHandle, &CreateSessionOptions, &SessionModification);
	if (Result == EOS_EResult::EOS_Success)
	{
		EOS_Sessions_UpdateSessionOptions UpdateSessionOptions;
		UpdateSessionOptions.ApiVersion = EOS_SESSIONS_UPDATESESSION_API_LATEST;
		UpdateSessionOptions.SessionModificationHandle = SessionModification;
		
		EOS_Sessions_UpdateSession(SessionsHandle, &UpdateSessionOptions, this, OnCreateSessionComplete);

		// Release the session modification handle
		EOS_SessionModification_Release(SessionModification);
	}
	else
	{
		UE_LOG(LogSessionSubsystem, Warning, TEXT("Failed to create session modification. Code: [%s]"), *FString(EOS_EResult_ToString(Result)));
	}
}

void USessionSubsystem::OnCreateSessionComplete(const EOS_Sessions_UpdateSessionCallbackInfo* Data)
{
	if(Data->ResultCode == EOS_EResult::EOS_Success)
	{
		UE_LOG(LogSessionSubsystem, Warning, TEXT("Session created successfully"));
	}
	else
	{
		UE_LOG(LogSessionSubsystem, Warning, TEXT("Failed to create session. Code: [%s]"), *FString(EOS_EResult_ToString(Data->ResultCode)));
	}
}

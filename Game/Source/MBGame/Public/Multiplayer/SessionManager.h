// Copyright © 2023 Melvin Brink


#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"

#include "SessionManager.generated.h"




//
// Custom delegates for other classes to bind callbacks to.
//

DECLARE_MULTICAST_DELEGATE_OneParam(FManagerOnCreateSessionComplete, bool bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_OneParam(FManagerOnJoinSessionComplete, FString Address);
DECLARE_MULTICAST_DELEGATE_TwoParams(FManagerOnFindSessionsComplete, bool bWasSuccessful, const TArray<FOnlineSessionSearchResult>& SessionResults);
DECLARE_MULTICAST_DELEGATE_OneParam(FManagerOnDestroySessionComplete, bool bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_OneParam(FManagerOnStartSessionComplete, bool bWasSuccessful);



/**
 * 
 */
UCLASS()
class MBGAME_API USessionManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	USessionManager();

	void CreateSession();
	void JoinSession(const FOnlineSessionSearchResult& SessionResult);
	void FindSessions();
	void DestroySession();
	void StartSession();
	
	// Custom delegates for other classes to bind callbacks to.
	FManagerOnCreateSessionComplete ManagerOnCreateSessionComplete;
	FManagerOnJoinSessionComplete ManagerOnJoinSessionComplete;
	FManagerOnFindSessionsComplete ManagerOnFindSessionsComplete;
	FManagerOnDestroySessionComplete ManagerOnDestroySessionComplete;
	FManagerOnStartSessionComplete ManagerOnStartSessionComplete;


protected:
	// Bound to the OnlineSessionInterface delegates.
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);


private:
	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;

	
	// OnlineSessionInterface delegates.
	FOnCreateSessionCompleteDelegate OnCreateSessionCompleteDelegate;
	FDelegateHandle OnCreateSessionCompleteDelegateHandle;
	
	FOnJoinSessionCompleteDelegate OnJoinSessionCompleteDelegate;
	FDelegateHandle OnJoinSessionCompleteDelegateHandle;

	FOnFindSessionsCompleteDelegate OnFindSessionsCompleteDelegate;
	FDelegateHandle OnFindSessionsCompleteDelegateHandle;
	
	FOnDestroySessionCompleteDelegate OnDestroySessionCompleteDelegate;
	FDelegateHandle OnDestroySessionCompleteHandle;
	
	FOnStartSessionCompleteDelegate OnStartSessionCompleteDelegate;
	FDelegateHandle OnStartSessionCompleteHandle;

	bool bCreateSessionOnDestroy = false;
};

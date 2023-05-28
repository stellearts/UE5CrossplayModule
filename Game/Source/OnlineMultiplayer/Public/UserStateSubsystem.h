// Copyright © 2023 Melvin Brink

#pragma once

#include "CoreMinimal.h"
#include "OnlineMultiplayer_CommonTypes.h"
#include "eos_common.h"
#include "eos_lobby_types.h"
#include "UserStateSubsystem.generated.h"



/**
 * Subsystem for keeping track of the local user's state.
 */
UCLASS()
class ONLINEMULTIPLAYER_API UUserStateSubsystem final : public UGameInstanceSubsystem
{
	GENERATED_BODY()


	
	// ------------------------------- Local user state -------------------------------
	 
	struct FLocalUser
	{
		std::string LobbyID;
		uint64 ShadowLobbyID = 0;
		EOS_ProductUserId ProductUserID;
		EOS_ContinuanceToken ContinuanceToken;
		EOS_EpicAccountId EpicAccountID;
		EOS_EExternalAccountType Platform;
	};
	FLocalUser LocalUser;
	
public:
	FORCEINLINE EOS_LobbyId GetLobbyID() const { return LocalUser.LobbyID.c_str(); }
	FORCEINLINE uint64 GetShadowLobbyID() const { return LocalUser.ShadowLobbyID; }
	FORCEINLINE EOS_EpicAccountId GetEpicAccountID() const { return LocalUser.EpicAccountID; }
	FORCEINLINE EOS_ProductUserId GetProductUserID() const { return LocalUser.ProductUserID; }
	FORCEINLINE EOS_EExternalAccountType GetPlatform() const { return LocalUser.Platform; }
	
	void SetLobbyID(const EOS_LobbyId InLobbyID) { LocalUser.LobbyID = InLobbyID; }
	void SetShadowLobbyID(const uint64 InShadowLobbyID) { LocalUser.ShadowLobbyID = InShadowLobbyID; }
	void SetEpicAccountId(const EOS_EpicAccountId EpicAccountId) { LocalUser.EpicAccountID = EpicAccountId; }
	void SetProductUserId(const EOS_ProductUserId ProductUserId) { LocalUser.ProductUserID = ProductUserId; }
	void SetContinuanceToken(const EOS_ContinuanceToken ContinuanceToken) { LocalUser.ContinuanceToken = ContinuanceToken; }
	void SetPlatform(const EOS_EExternalAccountType PlatformType) { LocalUser.Platform = PlatformType; }
	
	// Helper functions to make the code more readable.
	FORCEINLINE bool IsAuthLoggedIn() const { return EOS_EpicAccountId_IsValid(LocalUser.EpicAccountID) == EOS_TRUE; }
	FORCEINLINE bool IsConnectLoggedIn() const { return EOS_ProductUserId_IsValid(LocalUser.ProductUserID) == EOS_TRUE; }
	FORCEINLINE bool IsInLobby() const { return !LocalUser.LobbyID.empty(); }
	FORCEINLINE bool IsInShadowLobby() const { return LocalUser.ShadowLobbyID != 0; }




	
	// ------------------------------- Online user states -------------------------------

private:
	FOnlineUserMap Friends;
	FOnlineUserMap ConnectedUsers;

public:
	// Friends
	FORCEINLINE void AddFriend(const FOnlineUser OnlineUser){ Friends.Add(OnlineUser.ProductUserID, OnlineUser); }
	FORCEINLINE void RemoveFriend(const EOS_ProductUserId ProductUserId) { Friends.Remove(ProductUserId); }
	FORCEINLINE void FindFriend(const EOS_ProductUserId ProductUserId) { Friends.Find(ProductUserId); }
	
	// Connected users
	FORCEINLINE void AddConnectedUser(const FOnlineUser OnlineUser){ ConnectedUsers.Add(OnlineUser.ProductUserID, OnlineUser); }
	FORCEINLINE void RemoveConnectedUser(const EOS_ProductUserId ProductUserId) { ConnectedUsers.Remove(ProductUserId); }
	FORCEINLINE void FindConnectedUser(const EOS_ProductUserId ProductUserId) { ConnectedUsers.Find(ProductUserId); }
};
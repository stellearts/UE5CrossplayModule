// Copyright © 2023 Melvin Brink

#pragma once
#include "eos_common.h"

DECLARE_LOG_CATEGORY_EXTERN(LogUserType, Log, All);
inline DEFINE_LOG_CATEGORY(LogUserType);



inline FString EosProductIDToString(const EOS_ProductUserId& ProductUserID)
{
	char IDBuffer[EOS_PRODUCTUSERID_MAX_LENGTH + 1] = {};
	int32_t BufferSize = EOS_PRODUCTUSERID_MAX_LENGTH + 1;
	if(const EOS_EResult ResultCode = EOS_ProductUserId_ToString(ProductUserID, IDBuffer, &BufferSize); ResultCode != EOS_EResult::EOS_Success)
	{
		UE_LOG(LogUserType, Warning, TEXT("EOS_ProductUserId_ToString failed. Result Code: [%s]"), *FString(EOS_EResult_ToString(ResultCode)));
		return FString();
	}
	return FString(ANSI_TO_TCHAR(IDBuffer));
}

inline FString EosAccountIDToString(const EOS_EpicAccountId& EpicAccountID)
{
	char IDBuffer[EOS_EPICACCOUNTID_MAX_LENGTH + 1] = {};
	int32_t BufferSize = EOS_EPICACCOUNTID_MAX_LENGTH + 1;
	if(const EOS_EResult ResultCode = EOS_EpicAccountId_ToString(EpicAccountID, IDBuffer, &BufferSize); ResultCode != EOS_EResult::EOS_Success)
	{
		UE_LOG(LogUserType, Warning, TEXT("EOS_EpicAccountId_ToString failed. Result Code: [%s]"), *FString(EOS_EResult_ToString(ResultCode)));
		return FString();
	}
	return FString(ANSI_TO_TCHAR(IDBuffer));
}


FORCEINLINE EOS_ProductUserId EosProductIDFromString(const FString& ProductUserID)
{
	return EOS_ProductUserId_FromString(TCHAR_TO_UTF8(*ProductUserID));
}

FORCEINLINE EOS_EpicAccountId EosAccountIDFromString(const FString& EpicAccountID)
{
	return EOS_EpicAccountId_FromString(TCHAR_TO_UTF8(*EpicAccountID));
}

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"

/** Main log category used across the project */
DECLARE_LOG_CATEGORY_EXTERN(LogCrazyRoo, Log, All);

class FCrazyRooDebug
{
public:
	// 1. 현재 컴퓨터가 서버인지 클라이언트인지 명찰을 반환하는 함수
	static FString GetNetRole(const UObject* ContextObject);

	// 2. 화면과 로그창에 동시에 예쁘게 메시지를 띄워주는 함수
	static void Print(const UObject* ContextObject, const FString& Message, FColor Color = FColor::White, float TimeToDisplay = 5.0f);

	static void PrintSystem(const UObject* ContextObject, const FString& Message, FColor Color = FColor::White, float TimeToDisplay = 5.0f);
};

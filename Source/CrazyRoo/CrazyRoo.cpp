// Copyright Epic Games, Inc. All Rights Reserved.

#include "CrazyRoo.h"
#include "Modules/ModuleManager.h"
#include "UObject/Package.h"

IMPLEMENT_PRIMARY_GAME_MODULE( FDefaultGameModuleImpl, CrazyRoo, "CrazyRoo" );

DEFINE_LOG_CATEGORY(LogCrazyRoo)

FString FCrazyRooDebug::GetNetRole(const UObject* ContextObject)
{
	if (!ContextObject || !ContextObject->GetWorld())
	{
		return TEXT("[알수없음]");
	}

	UWorld* World = ContextObject->GetWorld();
	ENetMode NetMode = World->GetNetMode();

	if (NetMode == NM_DedicatedServer) return TEXT("[데디서버]");
	if (NetMode == NM_ListenServer)    return TEXT("[호스트]");
	if (NetMode == NM_Standalone)      return TEXT("[솔로]");

	if (NetMode == NM_Client)
	{
		//  현재 열려있는 월드의 클라이언트 번호를 가져옵니다.
		int32 ClientID = World->GetOutermost()->GetPIEInstanceID();
		return FString::Printf(TEXT("[손님-%d]"), ClientID);
	}

	return TEXT("[알수없음]");
}

void FCrazyRooDebug::Print(const UObject* ContextObject, const FString& Message, FColor Color, float TimeToDisplay)
{
	// 명찰 + 메시지 결합
	FString FinalMessage = FString::Printf(TEXT("%s %s"), *GetNetRole(ContextObject), *Message);

	// 출력 로그
	UE_LOG(LogCrazyRoo, Warning, TEXT("%s"), *FinalMessage);

	// 게임 화면(Viewport) 좌측 상단에 출력 (데디 서버는 화면이 없으므로 자동 패스)
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, TimeToDisplay, Color, FinalMessage);
	}
}

// 사용법
//FCrazyRooDebug::Print(this, TEXT("플레이어가 서버 접속!"), FColor::Cyan);
//FCrazyRooDebug::Print(this, TEXT("방이 깨짐"), FColor::Red, 10.0f);

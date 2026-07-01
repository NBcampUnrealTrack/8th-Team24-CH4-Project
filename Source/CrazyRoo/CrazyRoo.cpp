// Copyright Epic Games, Inc. All Rights Reserved.

#include "CrazyRoo.h"
#include "Modules/ModuleManager.h"
#include "UObject/Package.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Player/CrazyRooPlayerState.h"

IMPLEMENT_PRIMARY_GAME_MODULE( FDefaultGameModuleImpl, CrazyRoo, "CrazyRoo" );

DEFINE_LOG_CATEGORY(LogCrazyRoo)

static ACrazyRooPlayerState* FindPlayerStateFromContext(const UObject* ContextObject)
{
	if (const ACrazyRooPlayerState* PS = Cast<ACrazyRooPlayerState>(ContextObject))
	{
		return const_cast<ACrazyRooPlayerState*>(PS);
	}
	if (const APlayerController* PC = Cast<APlayerController>(ContextObject))
	{
		return PC->GetPlayerState<ACrazyRooPlayerState>();
	}
	if (const APawn* Pawn = Cast<APawn>(ContextObject))
	{
		return Pawn->GetPlayerState<ACrazyRooPlayerState>();
	}
	return nullptr;
}

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
		// 서버가 부여한 접속 순번(JoinOrder) 사용
		if (const ACrazyRooPlayerState* PS = FindPlayerStateFromContext(ContextObject))
		{
			const int32 Order = PS->GetJoinOrder();
			if (Order > 0)
			{
				return FString::Printf(TEXT("[Player%d]:"), Order);
			}
		}

		// 아직 JoinOrder가 리플리케이트 안 됐거나 PlayerState를 못 찾은 경우
		return TEXT("[Player(동기화 중)]:");
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

void FCrazyRooDebug::PrintSystem(const UObject* ContextObject, const FString& Message, FColor Color,
	float TimeToDisplay)
{
	// 시스템 로그는 "누구 것인지"를 따지지 않고, 서버/클라 여부만 표시
	FString RolePrefix = TEXT("[시스템]");
	if (ContextObject && ContextObject->GetWorld())
	{
		ENetMode NetMode = ContextObject->GetWorld()->GetNetMode();
		if (NetMode == NM_DedicatedServer) RolePrefix = TEXT("[서버]");
		else if (NetMode == NM_Client)      RolePrefix = TEXT("[클라]");
	}

	FString FinalMessage = FString::Printf(TEXT("%s %s"), *RolePrefix, *Message);

	UE_LOG(LogCrazyRoo, Warning, TEXT("%s"), *FinalMessage);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, TimeToDisplay, Color, FinalMessage);
	}
}

// 사용법
//FCrazyRooDebug::Print(this, TEXT("플레이어가 서버 접속!"), FColor::Cyan);
//FCrazyRooDebug::Print(this, TEXT("방이 깨짐"), FColor::Red, 10.0f);

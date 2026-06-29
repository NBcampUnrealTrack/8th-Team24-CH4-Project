// Copyright Epic Games, Inc. All Rights Reserved.

#include "CrazyRooGameMode.h"
#include "CrazyRoo.h"
#include "System/CrazyRooGameState.h"
#include "Player/CrazyRooPlayerState.h"
#include "CrazyRooPlayerController.h"
#include "GameFramework/PlayerController.h"


ACrazyRooGameMode::ACrazyRooGameMode()
{
	// stub
	GameStateClass        = ACrazyRooGameState::StaticClass();
	PlayerStateClass      = ACrazyRooPlayerState::StaticClass();
	PlayerControllerClass = ACrazyRooPlayerController::StaticClass();
	bUseSeamlessTravel    = true;
}

void ACrazyRooGameMode::BeginPlay()
{
	Super::BeginPlay();
	FCrazyRooDebug::Print(this, TEXT("GameMode BeginPlay (서버 시작됨)"), FColor::Cyan);
}

void ACrazyRooGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// 데디 서버 기준: 접속한 PC 는 모두 원격이므로 IsLocal 은 0 이어야 정상
	const int32 Num = GetNumPlayers();
	const FString Msg = FString::Printf(TEXT("PostLogin: 접속 인원 = %d (PC=%s, IsLocal=%d)"),
		Num,
		NewPlayer ? *NewPlayer->GetName() : TEXT("null"),
		(NewPlayer && NewPlayer->IsLocalController()) ? 1 : 0);

	FCrazyRooDebug::Print(this, Msg, FColor::Green);

}

void ACrazyRooGameMode::Logout(AController* Exiting)
{

	const FString Msg = FString::Printf(TEXT("Logout: %s 나감"),Exiting ? *Exiting->GetName() : TEXT("null"));

	FCrazyRooDebug::Print(this, Msg, FColor::Orange);

	Super::Logout(Exiting);
}

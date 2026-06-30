	// Fill out your copyright notice in the Description page of Project Settings.


#include "System/CrazyRooGameState.h"
#include "CrazyRoo.h"

void ACrazyRooGameState::BeginPlay()
{
	Super::BeginPlay();

	// GameState 는 서버 + 모든 클라에 존재 → 서버 로그 1번 + 각 클라 화면 1번씩
	FCrazyRooDebug::Print(this, TEXT("GameState BeginPlay"), FColor::White);
}

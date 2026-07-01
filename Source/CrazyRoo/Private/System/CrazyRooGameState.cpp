	// Fill out your copyright notice in the Description page of Project Settings.


#include "System/CrazyRooGameState.h"
#include "CrazyRoo.h"
#include "CrazyRooTypes.h"
#include "Net/UnrealNetwork.h"

ACrazyRooGameState::ACrazyRooGameState()
{
	CurrentMapId = NAME_None;
}

void ACrazyRooGameState::BeginPlay()
{
	Super::BeginPlay();

	// GameState 는 서버 + 모든 클라에 존재 → 서버 로그 1번 + 각 클라 화면 1번씩
	FCrazyRooDebug::Print(this, TEXT("GameState BeginPlay"), FColor::White);
}

void ACrazyRooGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACrazyRooGameState, CurrentMapId);
}

FName ACrazyRooGameState::GetCurrentMapId() const
{
	return CurrentMapId;
}

void ACrazyRooGameState::SetCurrentMapId(FName NewMapId)
{
	if (!HasAuthority() || CurrentMapId == NewMapId)
	{
		return;
	}

	CurrentMapId = NewMapId;

	FCrazyRooDebug::Print(this, FString::Printf(
		TEXT("[SERVER] CurrentMapId -> %s"), *CurrentMapId.ToString()), FColor::White);

	// 서버 자신은 OnRep이 자동으로 안 불리므로 직접 브로드캐스트
	OnCurrentMapChanged.Broadcast(CurrentMapId);
}

void ACrazyRooGameState::OnRep_CurrentMapId()
{
	FCrazyRooDebug::Print(this, FString::Printf(TEXT("[CLIENT] OnRep_CurrentMapId = %s"), *CurrentMapId.ToString()), FColor::White);

	OnCurrentMapChanged.Broadcast(CurrentMapId);
}

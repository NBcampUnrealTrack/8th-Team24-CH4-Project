	// Fill out your copyright notice in the Description page of Project Settings.


#include "System/CrazyRooGameState.h"
#include "CrazyRoo.h"
#include "CrazyRooTypes.h"
#include "Net/UnrealNetwork.h"

ACrazyRooGameState::ACrazyRooGameState()
{
	StageState = ECrazyRooStageState::RoleSelection;
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
	DOREPLIFETIME(ACrazyRooGameState, StageState);
}

ECrazyRooStageState ACrazyRooGameState::GetStageState() const
{
	return StageState;
}

void ACrazyRooGameState::SetStageState(ECrazyRooStageState NewState)
{
	if (!HasAuthority() || StageState == NewState)
	{
		return;
	}

	StageState = NewState;

	// 서버(데디) 로그
	FCrazyRooDebug::Print(this, FString::Printf(
		TEXT("[SERVER] SetMatchState -> %s"), CrazyRooMatchStateToString(NewState)), FColor::White);

	// 클라이언트는 OnRep_MatchState가 자동으로 호출되어 델리게이트를 뿌리지만,
	// 값을 직접 변경한 주체인 서버에서는 OnRep이 자동으로 호출되지 않음.
	// 따라서 서버 쪽 UI와 로직도 업데이트될 수 있도록 여기서 직접 델리게이트를 브로드캐스트해 줍니다.
	OnStageStateChanged.Broadcast(StageState);
}

void ACrazyRooGameState::OnRep_StageState()
{
	// 클라이언트 측에서 서버로부터 새로운 상태를 성공적으로 전달받았을 때 로그를 출력합니다.
	FCrazyRooDebug::Print(this, FString::Printf(
	TEXT("[CLIENT] OnRep_MatchState = %s"), CrazyRooMatchStateToString(StageState)), FColor::White);

	// 클라이언트의 UI 등이 이 델리게이트를 듣고 알맞은 화면으로 전환할 수 있도록 알림
	OnStageStateChanged.Broadcast(StageState);
}

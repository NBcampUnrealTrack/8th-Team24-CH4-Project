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

// Server 버튼으로 역할 요청 → 판정. 성공 true / 거부 false
bool ACrazyRooGameMode::RequestRole(APlayerController* Player, ECrazyRooPlayerRole DesiredRole)
{
	ACrazyRooPlayerState* PS = Player ? Player->GetPlayerState<ACrazyRooPlayerState>() : nullptr;
	if (!PS)
	{
		return false;
	}

	// 이미 내가 그 역할이면 성공 처리
	if (PS->GetPlayerRole() == DesiredRole)
	{
		return true;
	}

	// 비우기(None) 요청 (역할 해제 버튼? 그런거임)
	if (DesiredRole == ECrazyRooPlayerRole::None)
	{
		PS->SetPlayerRole(ECrazyRooPlayerRole::None);
		return true;
	}

	// 다른 사람이 이미 쓰는 역할이면 거부
	if (IsRoleTaken(DesiredRole))
	{
		const FString Msg = FString::Printf(TEXT("[SERVER] RequestRole 거부: %s 는 이미 사용중 (요청자=%s)"),
			CrazyRooRoleToString(DesiredRole), *PS->GetName());
		FCrazyRooDebug::Print(this, Msg, FColor::Red);
		return false;
	}

	// 승인 → 변경 (복제됨)
	PS->SetPlayerRole(DesiredRole);
	const FString Msg = FString::Printf(TEXT("[SERVER] RequestRole 승인: %s -> %s"),
		*PS->GetName(), CrazyRooRoleToString(DesiredRole));
	FCrazyRooDebug::Print(this, Msg, FColor::Green);
	return true;
}

// P_Left → P_Right → P_Shooter → P_Acrobat 순서로 빈 역할 반환
ECrazyRooPlayerRole ACrazyRooGameMode::FindFreeRole() const
{
	const ECrazyRooPlayerRole Order[] = {
		ECrazyRooPlayerRole::P_Left,
		ECrazyRooPlayerRole::P_Right,
		ECrazyRooPlayerRole::P_Shooter,
		ECrazyRooPlayerRole::P_Acrobat
	};
	for (ECrazyRooPlayerRole Candidate : Order)
	{
		if (!IsRoleTaken(Candidate))
		{
			return Candidate;
		}
	}
	return ECrazyRooPlayerRole::None;
}

// PlayerArray 를 돌며 그 역할 쓰는 사람이 있는지 검사
bool ACrazyRooGameMode::IsRoleTaken(ECrazyRooPlayerRole PlayerRole) const
{
	if (PlayerRole == ECrazyRooPlayerRole::None)
	{
		return false;
	}
	if (const AGameStateBase* GS = GetGameState<AGameStateBase>())
	{
		for (APlayerState* RawPS : GS->PlayerArray)
		{
			const ACrazyRooPlayerState* PS = Cast<ACrazyRooPlayerState>(RawPS);
			if (PS && PS->GetPlayerRole() == PlayerRole)
			{
				return true;
			}
		}
	}
	return false;
}

// Copyright Epic Games, Inc. All Rights Reserved.

#include "CrazyRooGameMode.h"
#include "CrazyRooTypes.h"
#include "CrazyRoo.h"
#include "System/CrazyRooGameState.h"
#include "Player/CrazyRooPlayerState.h"
#include "CrazyRooPlayerController.h"
#include "GameFramework/PlayerController.h"
#include "System/CrazyRooGameInstance.h"


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

// 플레이어가 서버에 접속을 완료했을 때 실행됨
void ACrazyRooGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// 데디 서버 기준: 접속한 PC 는 모두 원격이므로 IsLocal 은 0 이어야 정상
	// 접속한 총 인원수와 현재 접속한 사람의 정보를 로그로 출력 (데디 서버 디버깅용)
	const int32 Num = GetNumPlayers();
	const FString Msg = FString::Printf(TEXT("PostLogin: 접속 인원 = %d (PC=%s, IsLocal=%d)"),
		Num,
		NewPlayer ? *NewPlayer->GetName() : TEXT("null"),
		(NewPlayer && NewPlayer->IsLocalController()) ? 1 : 0);

	FCrazyRooDebug::Print(this, Msg, FColor::Green);

}

// 플레이어가 방을 나갔을 때 실행됨
void ACrazyRooGameMode::Logout(AController* Exiting)
{

	// 누가 나갔는지 로그로 출력
	const FString Msg = FString::Printf(TEXT("Logout: %s 나감"),Exiting ? *Exiting->GetName() : TEXT("null"));
	FCrazyRooDebug::Print(this, Msg, FColor::Orange);

	Super::Logout(Exiting);
}

void ACrazyRooGameMode::HandleSeamlessTravelPlayer(AController*& C)
{
	Super::HandleSeamlessTravelPlayer(C);

	// 맵 이동 직후 플레이어가 새로 생성된 PlayerState을 들고오기 때문에 GameInstance에 저장되어 있는 ID따라 배정해줘야함
	if (ACrazyRooPlayerState* PS = C ? C->GetPlayerState<ACrazyRooPlayerState>() : nullptr)
	{
		// 안 부서지고 살아남은 GameInstance에서 역할을 꺼내옴
		if (UCrazyRooGameInstance* GI = GetGameInstance<UCrazyRooGameInstance>())
		{
			const ECrazyRooPlayerRole Saved = GI->GetSavedPlayerRole(PS->GetPlayerId());
			if (Saved != ECrazyRooPlayerRole::None)
			{
				// 복원 완료 이제 클라이언트들에게 역할이 다시 전송
				PS->SetPlayerRole(Saved);
			}
		}
	}

}

// [Server] 클라이언트가 컨트롤러를 통해 역할을 요청했을 때 판정하는 함수
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

	// 역할을 비우겠다(None)고 요청한 경우 처리
	if (DesiredRole == ECrazyRooPlayerRole::None)
	{
		PS->SetPlayerRole(ECrazyRooPlayerRole::None);

		//GameInstance에 플레이어 역할 저장
		if (UCrazyRooGameInstance* GI = GetGameInstance<UCrazyRooGameInstance>())
		{
			GI->SavePlayerRole(PS->GetPlayerId(), ECrazyRooPlayerRole::None);
		}
		return true;
	}

	// 다른 사람이 이미 역할을 배정해 사용하고 있는지 검사
	if (IsRoleTaken(DesiredRole))
	{
		// 접근시 로그를 띄우고 false(거부) 반환
		const FString Msg = FString::Printf(TEXT("[SERVER] RequestRole 거부: %s 는 이미 사용중 (요청자=%s)"),
			CrazyRooRoleToString(DesiredRole), *PS->GetName());
		FCrazyRooDebug::Print(this, Msg, FColor::Red);
		return false;
	}

	// -------------------------------------------------------------
	// 정상적으로 역할을 승인하는 경우 처리
	// -------------------------------------------------------------
	PS->SetPlayerRole(DesiredRole);

	//GameInstance에 플레이어 역할 저장
	PS->SetPlayerRole(DesiredRole);
	if (UCrazyRooGameInstance* GI = GetGameInstance<UCrazyRooGameInstance>())
	{
		GI->SavePlayerRole(PS->GetPlayerId(), DesiredRole);
	}

	//디버그 메시지
	const FString Msg = FString::Printf(TEXT("[SERVER] RequestRole 승인: %s -> %s"),
		*PS->GetName(), CrazyRooRoleToString(DesiredRole));
	FCrazyRooDebug::Print(this, Msg, FColor::Green);

	return true;
}

void ACrazyRooGameMode::ChangeMap(FName MapId)
{
	UWorld* World = GetWorld();
	UCrazyRooGameInstance* GI = GetGameInstance<UCrazyRooGameInstance>();
	if (!World || !GI)
	{
		return;
	}

	// 입력받은 MapId("Stage_1")에 해당하는 맵 에셋의 '절대 경로 문자열'을 꺼내옴
	const FString Path = GI->GetStageMapPath(MapId);
	if (Path.IsEmpty())
	{
		FCrazyRooDebug::Print(this, FString::Printf(TEXT("거부: 맵 '%s' 경로를 찾을 수 없음"), *MapId.ToString()), FColor::Red);
		return;
	}
	// 레벨 이동 시 로그
	FCrazyRooDebug::Print(this, FString::Printf(TEXT("맵 '%s' 로 이동: %s"), *MapId.ToString(), *Path), FColor::Cyan);

	//  방장이 맵을 이동하면서 클라이언트들을 다 끌고 가는 명령
	// "?listen" 을 맵 경로 뒤에 꼬리표처럼 붙여줍니다. (예: "/Game/Maps/Stage_1?listen")
	World->ServerTravel(Path + TEXT("?listen"));
	/*
	 *언리얼 엔진에서 유저 중 한 명이 방장이 되어 플레이하는 방식(호스트/리슨 서버 환경)일 때, 방장이 ServerTravel로 맵을 열면 엔진은 기본적으로 "어? 방장 혼자 싱글 플레이하러 가는구나!" 하고 착각해서 열어버립니다. 이러면 같이 가던 클라이언트들의 연결이 다 끊어지고 로비로 튕겨버립니다.
	 *이때 맵 경로 뒤에 ?listen 옵션을 붙여주면, 엔진에게 다음과 같이 강력하게 지시하는 효과가 있습니다.
	 *방장 혼자 가는게 아니라 새 맵으로 넘어가서도 계속 서버 통신 포트 열어두고(listen), 같이 넘어오는 클라이언트들을 계속 접속상태로 유지해
	 *즉, 이 함수는 데이터 테이블 기반의 맵 관리와 멀티플레이어 환경의 맵 이동 안전장치를 모두 코드
	 */
}

// 빈 역할을 찾아주는 함수입니다.
ECrazyRooPlayerRole ACrazyRooGameMode::FindFreeRole() const
{
	// 역할을 검사할 순서를 미리 배열에 저장
	const ECrazyRooPlayerRole Order[] = {
		ECrazyRooPlayerRole::P_Left,
		ECrazyRooPlayerRole::P_Right,
		ECrazyRooPlayerRole::P_Shooter,
		ECrazyRooPlayerRole::P_Acrobat
	};


	for (ECrazyRooPlayerRole Candidate : Order)
	{
		// 어떤 플레이어가 역할을 할당받았는지 확인
		if (!IsRoleTaken(Candidate))
		{
			//비어있으면 그 역할 즉시 반환
			return Candidate;
		}
	}

	// 다 배정되어 있으면 None을 반환합니다
	return ECrazyRooPlayerRole::None;
}

// 다른 플레이어가  이미 그 역할을 선점했는지 PlayerArray(접속자 목록)를 돌면서 확인하는 함수
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

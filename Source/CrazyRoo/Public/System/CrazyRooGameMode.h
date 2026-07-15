// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Types/CrazyRooTypes.h"
#include "CrazyRooGameMode.generated.h"


enum class ECrazyRooPlayerRole : uint8;
class ASharedCharacterManager; //공유 매니저 추가
/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class ACrazyRooGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	/** Constructor */
	ACrazyRooGameMode();
	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void Logout(AController* Exiting) override;
	virtual void HandleSeamlessTravelPlayer(AController*& C) override; // 심리스 트래블로 넘어온 플레이어를 처리하는 함수

	// [Server] 역할 요청 판정. 성공 true / 이미 역할이 배정되어 있으면 false
	bool RequestRole(APlayerController* Player, ECrazyRooPlayerRole DesiredRole);


	// [Server] MapId 에 해당하는 맵으로 모든 플레이어이동
	UFUNCTION(BlueprintCallable, Category = "CrazyRoo")
	void ChangeMap(FName MapId);

	/** 게임플레이 맵에서만 존재. 로비에선 nullptr */
	ASharedCharacterManager* GetSharedCharacterManager() const;

	/** 게임플레이 맵에 플레이어가 진입했을 때 공통 세팅 (카메라 + 역할 폴백). 로비 맵이면 아무것도 안 함 */
	void InitPlayerForCurrentMap(AController* C);


	/** [Server] 시작 조건 검사: 최소 인원 충족 + (방장 제외) 전원 준비 */
	bool AreAllPlayersReady() const;

	/** 개발용 솔로 시작 허용 여부 (ServerTravel 가드가 참조) */
	bool IsDevSoloStart() const;
	// ★ [추가 끝] ────────────────────────────────────────────

	// [한길] 0714 QTE후 기존 방식 그대로 플레이 이어나가게 설정.
	/** 현재 게임플레이 시작 방식을 반환합니다. */
	ECrazyRooGameStartMode GetGameStartMode() const { return GameStartMode; }

protected:

	// 다음 비어있는 역할 반환 (없으면 None)
	// P1->P2->P3->P4 순서대로 남는 자리를 찾아주는 함수입니다.
	ECrazyRooPlayerRole FindFreeRole() const;

	// 특정 역할이 이미 배정됐는지
	// 방에 있는 플레이어를 돌아 검사해 겹치는 역할이 있는지 검사합니다.
	bool IsRoleTaken(ECrazyRooPlayerRole PlayerRole) const;

	/** GameInstance 의 CurrentMapId 로 데이터 테이블 찾아 MatchState 를 GameState 에 설정 */
	void ApplyMatchStateForCurrentMap();

	// 접속할 플레이어에게 부여할 번호 (1부터 증가) 삭제 가능
	int32 NextJoinOrder = 1;

	// protected에 추가
	UPROPERTY()
	ASharedCharacterManager* SharedCharacterManager = nullptr;


	/** 시작에 필요한 최소 인원. 테스트 2 → 완성 시 BP Class Defaults에서 4로 변경 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CrazyRoo")
	int32 MinPlayersToStart = 2;

	/** [Dev] true면 인원/준비 검사 생략 — 방장이 역할만 골랐으면 시작 가능.
	 *  정식 규칙으로 전환할 땐 이 값만 false로 (BP Class Defaults에서 체크 해제) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CrazyRoo|Dev")
	bool bDevAllowSoloStart = true;
	// ★ [추가 끝] ────────────────────────────────────────────

	// [한길] 0712
	// 게임플레이 맵 시작 방식을 선택합니다.
	// Normal은 기존처럼 카운트다운 후 AutoMove, 테스트 모드는 카운트다운 없이 즉시 원하는 이동 모드로 시작합니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CrazyRoo|Dev")
	ECrazyRooGameStartMode GameStartMode = ECrazyRooGameStartMode::Normal;

	// [한길] 0712
	// GameStartMode 값에 따라 카운트다운을 시작하거나 테스트용 이동 모드로 바로 전환합니다.
	void HandleGameStartMode();


	// -----------------------------
	// [한길] 0711 카운트다운 관련
	// -----------------------------

	// 1초마다 실행되는 타이머
	FTimerHandle CountdownTimer;

	// 시작 카운트다운 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CrazyRoo|Countdown")
	int32 CountdownSeconds = 5;

	// 현재 남은 시간
	int32 CurrentCountdown;

	// 카운트다운 시작
	void StartCountdown();

	// 1초마다 호출
	void CountdownTick();

	// 카운트다운 종료 후 게임 시작
	void StartGame();
};




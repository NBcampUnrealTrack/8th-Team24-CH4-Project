// SharedCharacterManager.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RunnerTypes.h"
#include "Player/SharedRailCharacter.h"
//#include "Dummy/RailTypes.h"
#include "SharedCharacterManager.generated.h"

class ASharedRailCharacter;
class ATurnJudgeBox;
enum class ECrazyRooPlayerRole : uint8;
class AFlyingEnemySpawner;
class AFlyingEnemy;
class ARunnerObstacleSpawner;
class ARunnerObstacle;
class AGroupQTEBreakableWall;

UCLASS()
class CRAZYROO_API ASharedCharacterManager :public AActor
{
	GENERATED_BODY()

public:
	ASharedCharacterManager();

	virtual void BeginPlay()override;

	// 서버에서 공유 캐릭터를 찾거나 저장한다
	void InitializeSharedCharacter();

	// PlayerController가 명령을 보낼 때 호출한다 (저가 만든 CrazyRooTypes로 사용해서 주석처리 하겠습니다)
	//void RequestMoveLeft(ERailPlayerRole RequestRole);
	//void RequestMoveRight(ERailPlayerRole RequestRole);
	//void RequestJump(ERailPlayerRole RequestRole);
	//void RequestSlide(ERailPlayerRole RequestRole);
	//void RequestFireToLocation(ERailPlayerRole RequestRole, const FVector& TargetLocation);
	//void RequestRotateCameraLeft(ERailPlayerRole RequestRole);
	//void RequestRotateCameraRight(ERailPlayerRole RequestRole);
	//void RequestFireAimRay(ERailPlayerRole RequestRole, const FVector& AimOrigin, const FVector& AimDirection);

	void RequestMoveLeft(ECrazyRooPlayerRole RequestRole);
	void RequestMoveRight(ECrazyRooPlayerRole RequestRole);
	void RequestJump(ECrazyRooPlayerRole RequestRole);
	void RequestSlide(ECrazyRooPlayerRole RequestRole);
	void RequestFireToLocation(ECrazyRooPlayerRole RequestRole, const FVector& TargetLocation);
	void RequestRotateCameraLeft(ECrazyRooPlayerRole RequestRole);
	void RequestRotateCameraRight(ECrazyRooPlayerRole RequestRole);
	void RequestFireAimRay(ECrazyRooPlayerRole RequestRole, const FVector& AimOrigin, const FVector& AimDirection, AController* ShooterController);
	// [한길] 0714 QTE
	// QTE 벽이 시작될 때 자신을 Manager에 등록함.
	void RegisterActiveQTEWall(AGroupQTEBreakableWall* Wall);
	// QTE 성공/실패 후 현재 활성 QTE 벽 등록을 해제함.
	void UnregisterActiveQTEWall(AGroupQTEBreakableWall* Wall);
	// PlayerController에서 넘어온 G키 입력을 현재 활성 QTE 벽으로 전달함.
	// 일반 입력 잠금 상태(bRunnerInputLocked)여도 QTE 입력은 허용함.
	// QTE 중에는 이동/점프/사격 등은 막히지만 G키 연타는 받아야 하기 때문.
	void RequestQTEPress(ECrazyRooPlayerRole RequestRole);

protected:
	// 맵에 배치된 BP_SharedRailCharacter를 저장
	UPROPERTY()
	ASharedRailCharacter* SharedCharacter = nullptr;

	// [한길] 0714 현재 진행중인 QTE 벽을 저장하는 포인터.
	UPROPERTY()
	AGroupQTEBreakableWall* ActiveQTEWall = nullptr;

	// [한길] 0703 코너링 시스템.
	// 현재 SharedCharacter가 들어가 있는 TurnJudgeBox를 찾습니다.
	// 없다면 nullptr을 반환합니다.
	ATurnJudgeBox* FindActiveTurnJudgeBox() const;
	// A/D 입력이 들어왔을 때
	// 코너링 박스 안이라면 코너링 판정을 수행합니다.
	// 성공 여부와 관계없이 박스 안이라면 true를 반환합니다.
	bool TryHandleTurnInput(ERunnerTurnInputDirection InputDirection);
	void StopFlyingEnemiesForRunnerTurn();

	// [한길] 0706 코너링 입력 한번으로 고정.
	// 이미 코너링 처리가 완료된 판정 박스
	// 이 박스 안에서는 다시 A/D를 눌러도 코너링을 처리하지 않습니다.
	UPROPERTY()
	TWeakObjectPtr<ATurnJudgeBox> ResolvedTurnJudgeBox;

	// ── 적 공격 이벤트 → 플레이어 HP 연결 ──
	UFUNCTION()
	void HandleEnemySpawned(AFlyingEnemySpawner* Spawner, AFlyingEnemy* SpawnedEnemy);

	UFUNCTION()
	void HandleEnemyAttackHitPlayer(AFlyingEnemy* Enemy, AActor* TargetPlayer, float Damage);

	// ── 장애물 충돌 이벤트 → 플레이어 HP 연결 ──
	UFUNCTION()
	void HandleObstacleSpawned(ARunnerObstacleSpawner* Spawner, ARunnerObstacle* SpawnedObstacle);

	// RunnerObstacle.h의 FRunnerObstaclePlayerOverlapSignature
	UFUNCTION()
	void HandlePlayerHitObstacle(ARunnerObstacle* Obstacle, AActor* PlayerActor,
		ERunnerObstacleActionType ActionType, ERunnerLaneCoverage LaneCoverage, ERunnerLane Lane);


public:
	// 테스트 목적으로 생성. true면 누구나 모든 입력 가능(Stand Alone으로 테스트용), false면 역할별 입력만 가능.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Debug")
	bool bIgnorePlayerRole = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Input")
	bool bRunnerInputLocked = false;

	UFUNCTION(BlueprintCallable, Category = "Runner|Input")
	void SetRunnerInputLocked(bool bLocked);

	// [한길] 0712
	// 테스트 시작 모드에서 역할 배정을 무시하고 모든 플레이어 입력을 허용할지 설정합니다.
	UFUNCTION(BlueprintCallable, Category = "Debug")
	void SetIgnorePlayerRole(bool bShouldIgnorePlayerRole);

	// [한길] 0703 테스트용 앞뒤 움직임.
	void RequestMove(float AxisValue);

	// [한길] 0707 GameMode나 UI, 디버그시 이동 모드 쉽게 변경 가능하게 하기 위한 함수들입니다.
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetSharedCharacterMoveMode(ERunnerMoveMode NewMoveMode);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void StartAutoMove();

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void StartManualMove();

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void StopMove();

	// [한길] 0710 5초 딜레이 스타트
	/**
 * 일정 시간 동안 대기한 뒤 공유 캐릭터의 자동 전진을 시작합니다.
 * 서버에서만 실행되어야 합니다.
 */
	void StartAutoMoveAfterDelay();

protected:
	// 게임 시작 후 자동 전진까지 기다릴 시간입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Start")
	float AutoMoveStartDelay = 5.0f;

	// 시작 지연 타이머를 관리합니다.
	FTimerHandle AutoMoveStartTimerHandle;

	// 타이머가 끝났을 때 실제로 AutoMove로 변경하는 함수입니다.
	void HandleAutoMoveStart();
};

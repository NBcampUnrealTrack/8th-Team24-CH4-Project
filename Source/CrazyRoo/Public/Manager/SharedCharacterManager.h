// SharedCharacterManager.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RunnerTypes.h"
//#include "Dummy/RailTypes.h"
#include "SharedCharacterManager.generated.h"

class ASharedRailCharacter;
class ATurnJudgeBox;
enum class ECrazyRooPlayerRole : uint8;
class AFlyingEnemySpawner;
class AFlyingEnemy;

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
	void RequestFireAimRay(ECrazyRooPlayerRole RequestRole, const FVector& AimOrigin, const FVector& AimDirection);

protected:
	// 맵에 배치된 BP_SharedRailCharacter를 저장
	UPROPERTY()
	ASharedRailCharacter* SharedCharacter = nullptr;

	// [한길] 0703 코너링 시스템.
	// 현재 SharedCharacter가 들어가 있는 TurnJudgeBox를 찾습니다.
	// 없다면 nullptr을 반환합니다.
	ATurnJudgeBox* FindActiveTurnJudgeBox() const;
	// A/D 입력이 들어왔을 때
	// 코너링 박스 안이라면 코너링 판정을 수행합니다.
	// 성공 여부와 관계없이 박스 안이라면 true를 반환합니다.
	bool TryHandleTurnInput(ERunnerTurnInputDirection InputDirection);

	// ── 적 공격 이벤트 → 플레이어 HP 연결 ──
	UFUNCTION()
	void HandleEnemySpawned(AFlyingEnemySpawner* Spawner, AFlyingEnemy* SpawnedEnemy);

	UFUNCTION()
	void HandleEnemyAttackHitPlayer(AFlyingEnemy* Enemy, AActor* TargetPlayer, float Damage);


public:
	// 테스트 목적으로 생성. true면 누구나 모든 입력 가능(Stand Alone으로 테스트용), false면 역할별 입력만 가능.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Debug")
	bool bIgnorePlayerRole = false;

	// [한길] 0703 테스트용 앞뒤 움직임.
	void RequestMove(float AxisValue);
};

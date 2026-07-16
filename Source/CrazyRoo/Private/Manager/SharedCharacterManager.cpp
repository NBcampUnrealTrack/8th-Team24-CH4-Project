// SharedCharacterManager.cpp

#include "Manager/SharedCharacterManager.h"
#include "Player/SharedRailCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Types/CrazyRooTypes.h"
#include "CrazyRoo.h"
#include "Cornering/TurnJudgeBox.h"
#include "Enemies/FlyingEnemySpawner.h"
#include "Enemies/FlyingEnemy.h"
#include "Obstacles/GroupQTEBreakableWall.h"
#include "Obstacles/RunnerObstacleSpawner.h"
#include "Obstacles/RunnerObstacle.h"
#include "TimerManager.h"
#include "System/CrazyRooGameState.h"

ASharedCharacterManager::ASharedCharacterManager()
{
	PrimaryActorTick.bCanEverTick = false;

	// Manager는 서버에서만 명령 처리용으로 사용
	bReplicates = false;
}

void ASharedCharacterManager::BeginPlay()
{
    Super::BeginPlay();

	InitializeSharedCharacter();

	// [Server] 맵 내 모든 적 스포너 탐색 및 스폰 이벤트 바인딩
	if (HasAuthority())
	{
		TArray<AActor*> Spawners;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFlyingEnemySpawner::StaticClass(), Spawners);

		// 각 스포너의 몬스터 생성 이벤트 구독
		for (AActor* Actor : Spawners)
		{
			if (AFlyingEnemySpawner* Spawner = Cast<AFlyingEnemySpawner>(Actor))
			{
				Spawner->OnFlyingEnemySpawned.AddDynamic(this, &ASharedCharacterManager::HandleEnemySpawned);
			}
		}

		UE_LOG(LogCrazyRoo, Warning, TEXT("[Manager] 적 스포너 %d개 구독 완료"), Spawners.Num());

		// 장애물 스포너 바인딩 (앞으로 나올 장애물용)
		// 장애물을 다 부수면 10초 뒤에 새로운 장애물이 다시 리스폰된다 or 특정 조건이 되면 추가 장애물을 소환한다라는 기믹이 생긴다면 필요
		//TArray<AActor*> ObstacleSpawners;
		//UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARunnerObstacleSpawner::StaticClass(), ObstacleSpawners);
		//for (AActor* Actor : ObstacleSpawners)
		//{
		//	if (ARunnerObstacleSpawner* Spawner = Cast<ARunnerObstacleSpawner>(Actor))
		//	{
		//		Spawner->OnObstacleSpawned.AddDynamic(this, &ASharedCharacterManager::HandleObstacleSpawned);
		//	}
		//}
		//UE_LOG(LogCrazyRoo, Warning, TEXT("[Manager] 장애물 스포너 %d개 구독 완료"), ObstacleSpawners.Num());

		// =========================================================================
		// 이미 맵에 스폰되어 버린 장애물들 직접 구독
		// =========================================================================
		TArray<AActor*> ExistingObstacles;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARunnerObstacle::StaticClass(), ExistingObstacles);

		for (AActor* Actor : ExistingObstacles)
		{
			if (ARunnerObstacle* Obstacle = Cast<ARunnerObstacle>(Actor))
			{
				// 이미 맵에 존재하는 장애물을 찾아 플레이어 피격 이벤트를 직접 묶어줍니다.
				Obstacle->OnPlayerHitObstacle.AddUniqueDynamic(this, &ASharedCharacterManager::HandlePlayerHitObstacle);
			}
		}
		UE_LOG(LogCrazyRoo, Warning, TEXT("[Manager] 장애물 %d개 구독 완료"), ExistingObstacles.Num());
	}

}

void ASharedCharacterManager::InitializeSharedCharacter()
{
	if (!HasAuthority())
    {
		return;
    }

	// 맵에 배치된 SharedRailCharacter 하나를 찾는다
	SharedCharacter =Cast<ASharedRailCharacter>(
	        UGameplayStatics::GetActorOfClass(
					GetWorld(),
        ASharedRailCharacter::StaticClass()
	        )
	    );

	if (SharedCharacter)
    {
		UE_LOG(LogTemp,Warning,TEXT("Manager found SharedCharacter: %s"),
		*SharedCharacter->GetName());
    }
	else
    {
		UE_LOG(LogTemp,Error,TEXT("Manager could not find SharedCharacter"));
    }

	// [한길] 0712
	// 시작 방식과 카운트다운은 GameMode의 GameStartMode에서 결정합니다.
	// 여기서 별도 타이머를 걸면 ManualTest 중에도 나중에 AutoMove로 바뀔 수 있으므로 자동 시작은 하지 않습니다.
}
//[서희]
void ASharedCharacterManager::NotifySharedKeyPressed(
	ECrazyRooSharedKey Key
)
{

	if (!HasAuthority())
	{
		return;
	}

	ACrazyRooGameState* GameState =
		GetWorld()
			? GetWorld()->GetGameState<ACrazyRooGameState>()
			: nullptr;

	if (!GameState)
	{
		return;
	}

	GameState->NotifySharedKeyPressed(Key);
}

void ASharedCharacterManager::RequestMoveLeft(ECrazyRooPlayerRole RequestRole)
{
	if (!HasAuthority() || !SharedCharacter || bRunnerInputLocked)
	{
		return;
	}

	// None은 테스트 모드여도 차단
	//if (RequestRole == ECrazyRooPlayerRole::None && !bIgnorePlayerRole)
	//{
	//	return;
	//}

	if (!bIgnorePlayerRole && RequestRole != ECrazyRooPlayerRole::P_Left)
	{
		UE_LOG(LogCrazyRoo, Warning, TEXT("MoveLeft 무시: 요청역할 %s (P_Left 아님)"),CrazyRooRoleToString(RequestRole));
		return;
	}

	// 승인된 A 입력을 모든 클라이언트 HUD에 전달
	NotifySharedKeyPressed(ECrazyRooSharedKey::A);

	// [한길] 0703 코너링 시스템.
	// 먼저 코너링 판정을 시도합니다.
	if (TryHandleTurnInput(
		ERunnerTurnInputDirection::LeftInput))
	{
		// 코너링 박스 안이었다면
		// 일반 레일 이동을 하지 않습니다.
		return;
	}

	// 코너링 박스 밖이라면
	// 기존 레일 이동을 수행합니다.
	SharedCharacter->MoveLeft();
}

void ASharedCharacterManager::RequestMoveRight(ECrazyRooPlayerRole RequestRole)
{
	if (!HasAuthority() || !SharedCharacter || bRunnerInputLocked)
	{
		return;
	}


	if (!bIgnorePlayerRole && RequestRole != ECrazyRooPlayerRole::P_Right)
	{
		UE_LOG(LogCrazyRoo, Warning, TEXT("MoveRight 무시: 요청역할 %s (P_Right 아님)"),CrazyRooRoleToString(RequestRole));
		return;
	}

	NotifySharedKeyPressed(ECrazyRooSharedKey::D);

	//[한길] 0703 코너링 시스템.
	// 먼저 우회전 판정을 수행합니다.
	if (TryHandleTurnInput(
		ERunnerTurnInputDirection::RightInput))
	{
		return;
	}

	// 코너링 박스가 아니면
	// 기존 레일 이동을 수행합니다.
	SharedCharacter->MoveRight();
}

void ASharedCharacterManager::RequestJump(ECrazyRooPlayerRole RequestRole)
{
	if (!HasAuthority() || !SharedCharacter || bRunnerInputLocked)
	{
		return;
	}


	if (!bIgnorePlayerRole && RequestRole != ECrazyRooPlayerRole::P_Acrobat)
	{
		UE_LOG(LogCrazyRoo, Warning, TEXT("Jump 무시: 요청역할 %s (P_Acrobat 아님)"),CrazyRooRoleToString(RequestRole));
		return;
	}

	NotifySharedKeyPressed(ECrazyRooSharedKey::Space);

	SharedCharacter->DoJump();
}

void ASharedCharacterManager::RequestSlide(ECrazyRooPlayerRole RequestRole)
{
	if (!HasAuthority() || !SharedCharacter || bRunnerInputLocked)
	{
		return;
	}

	if (!bIgnorePlayerRole && RequestRole != ECrazyRooPlayerRole::P_Acrobat)
	{
		UE_LOG(LogCrazyRoo, Warning, TEXT("Slide 무시: 요청역할 %s (P_Acrobat 아님)"),CrazyRooRoleToString(RequestRole));
		return;
	}

	NotifySharedKeyPressed(ECrazyRooSharedKey::Shift);

	SharedCharacter->DoSlide();
}

void ASharedCharacterManager::RequestFireToLocation(ECrazyRooPlayerRole RequestRole, const FVector& TargetLocation)
{
	if (!HasAuthority() || !SharedCharacter || bRunnerInputLocked)
	{
		return;
	}

	if (!bIgnorePlayerRole && RequestRole != ECrazyRooPlayerRole::P_Shooter)
	{
		UE_LOG(LogCrazyRoo, Warning, TEXT("FireToLocation 무시: 요청역할 %s (P_Shooter 아님)"),CrazyRooRoleToString(RequestRole));
		return;
	}

	SharedCharacter->FireToLocation(TargetLocation);
}

void ASharedCharacterManager::RequestRotateCameraLeft(ECrazyRooPlayerRole RequestRole)
{
	if (!HasAuthority() || !SharedCharacter || bRunnerInputLocked)
	{
		return;
	}

	if (!bIgnorePlayerRole && RequestRole != ECrazyRooPlayerRole::P_Shooter)
	{
		UE_LOG(LogCrazyRoo, Warning, TEXT("CameraLeft 무시: 요청역할 %s (P_Shooter 아님)"),CrazyRooRoleToString(RequestRole));
		return;
	}

	NotifySharedKeyPressed(ECrazyRooSharedKey::Q);

	SharedCharacter->RotateCameraLeft();
}

void ASharedCharacterManager::RequestRotateCameraRight(ECrazyRooPlayerRole RequestRole)
{
	if (!HasAuthority() || !SharedCharacter || bRunnerInputLocked)
	{
		return;
	}

	if (!bIgnorePlayerRole && RequestRole != ECrazyRooPlayerRole::P_Shooter)
	{
		UE_LOG(LogCrazyRoo, Warning, TEXT("CameraRight 무시: 요청역할 %s (P_Shooter 아님)"),CrazyRooRoleToString(RequestRole));
		return;
	}

	NotifySharedKeyPressed(ECrazyRooSharedKey::E);

	SharedCharacter->RotateCameraRight();
}

void ASharedCharacterManager::RequestFireAimRay(ECrazyRooPlayerRole RequestRole, const FVector& AimOrigin, const FVector& AimDirection, AController* ShooterController)
{
	if (!HasAuthority() || !SharedCharacter || bRunnerInputLocked)
	{
		return;
	}

	if (!bIgnorePlayerRole && RequestRole != ECrazyRooPlayerRole::P_Shooter)
	{
		UE_LOG(LogCrazyRoo, Warning, TEXT("FireAimRay 무시: 요청역할 %s (P_Shooter 아님)"),CrazyRooRoleToString(RequestRole));
		return;
	}

	// 승인된 마우스 사격 입력을 HUD에 전달
	NotifySharedKeyPressed(ECrazyRooSharedKey::Mouse);

	// [한길] 0710 발사 방식에 따라 발사 수행.
	SharedCharacter->FireBySelectedMode(AimOrigin, AimDirection, ShooterController);
}

// [한길] 0714 QTE
void ASharedCharacterManager::RegisterActiveQTEWall(AGroupQTEBreakableWall* Wall)
{
	if (!HasAuthority())
	{
		return;
	}

	// 이후 들어오는 QTE 입력을 이 벽 액터로 전달합니다.
	ActiveQTEWall = Wall;
}

void ASharedCharacterManager::UnregisterActiveQTEWall(AGroupQTEBreakableWall* Wall)
{
	if (!HasAuthority())
	{
		return;
	}

	if (ActiveQTEWall == Wall)
	{
		// 완료/실패한 QTE 벽은 더 이상 입력을 받지 않도록 비웁니다.
		ActiveQTEWall = nullptr;
	}
}

void ASharedCharacterManager::RequestQTEPress(ECrazyRooPlayerRole RequestRole)
{
	if (!HasAuthority() || !ActiveQTEWall)
	{
		return;
	}

	if (RequestRole == ECrazyRooPlayerRole::None)
	{
		if (!bIgnorePlayerRole)
		{
			return;
		}

		// 테스트 모드에서는 역할이 없어도 혼자 QTE를 확인할 수 있게 임시 역할로 기록합니다.
		RequestRole = ECrazyRooPlayerRole::P_Shooter;
	}

	// QTE 입력은 러너 일반 입력 잠금 중에도 허용합니다.
	ActiveQTEWall->HandleQTEPress(RequestRole);
}

ATurnJudgeBox* ASharedCharacterManager::FindActiveTurnJudgeBox() const
{
	// 공유 캐릭터가 없으면 검사할 수 없습니다.
	if (!SharedCharacter)
	{
		return nullptr;
	}

	// 현재 레벨에 존재하는 모든 TurnJudgeBox를 찾습니다.
	TArray<AActor*> FoundBoxes;

	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(),
		ATurnJudgeBox::StaticClass(),
		FoundBoxes
	);

	// 모든 TurnJudgeBox를 순회합니다.
	for (AActor* Actor : FoundBoxes)
	{
		ATurnJudgeBox* TurnBox = Cast<ATurnJudgeBox>(Actor);

		// SharedCharacter가 현재 박스 안에 있다면
		// 해당 TurnJudgeBox를 반환합니다.
		if (TurnBox &&
			TurnBox->IsPlayerInJudgeBox(SharedCharacter))
		{
			return TurnBox;
		}
	}

	// 현재 코너링 가능한 박스가 없습니다.
	return nullptr;
}

bool ASharedCharacterManager::TryHandleTurnInput(ERunnerTurnInputDirection InputDirection)
{
	// 서버에서만 처리합니다.
	if (!HasAuthority() || !SharedCharacter)
	{
		return false;
	}

	// 현재 플레이어가 들어가 있는 코너링 박스를 찾습니다.
	ATurnJudgeBox* TurnBox = FindActiveTurnJudgeBox();

	// 코너링 박스가 없으면 일반 레일 이동을 수행해야 하므로 false
	if (!TurnBox)
	{
		// [한길] 0706 박스 밖으로 나갔다면 다음 코너에서 다시 판정 가능하도록 초기화
		ResolvedTurnJudgeBox = nullptr;
		return false;
	}

	// [한길] 0706 이미 이 박스에서 코너링에 성공했다면 다시 처리하지 않음
	if (ResolvedTurnJudgeBox.IsValid() &&
		ResolvedTurnJudgeBox.Get() == TurnBox)
	{
		return true;
	}

	// TurnJudgeBox의 코너링 판정을 수행합니다.
	const bool bTurnSucceeded =
		TurnBox->TryResolveTurn(
			SharedCharacter,
			InputDirection
		);

	if (bTurnSucceeded)
	{
		// [한길] 0706 캐릭터 회전 성공시 판정 완료 박스로 변경.
		ResolvedTurnJudgeBox = TurnBox;

		// TurnJudgeBox에서 권장하는 회전 각도(-90 또는 +90)를 가져옵니다.
		const float YawDelta =
			TurnBox->GetRecommendedYawDeltaForInput(
				InputDirection);
		const FTransform SnapTransform = TurnBox->GetTurnSnapTransform();

		StopFlyingEnemiesForRunnerTurn();

		// 캐릭터를 회전시킵니다.
		SharedCharacter->ApplyCornerTurnAtSnapPoint(YawDelta, SnapTransform);

		UE_LOG(LogTemp, Warning, TEXT("Turn Succeeded"));
	}
	else
	{
		// 코너링 실패
		UE_LOG(LogTemp, Warning, TEXT("Turn Failed"));
	}

	// 코너링 박스 안에서는 일반 레일 이동을 하지 않도록 true를 반환합니다.
	return true;
}

void ASharedCharacterManager::StopFlyingEnemiesForRunnerTurn()
{
	if (!HasAuthority())
	{
		return;
	}

	TArray<AActor*> Spawners;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFlyingEnemySpawner::StaticClass(), Spawners);

	for (AActor* Actor : Spawners)
	{
		if (AFlyingEnemySpawner* Spawner = Cast<AFlyingEnemySpawner>(Actor))
		{
			Spawner->HandleRunnerTurnSucceeded();
		}
	}
}

//[한길] 0703 테스트용 앞뒤 움직임.
void ASharedCharacterManager::RequestMove(float AxisValue)
{
	if (!HasAuthority() || !SharedCharacter || bRunnerInputLocked)
	{
		return;
	}

	SharedCharacter->DebugMoveForward(AxisValue);
}

void ASharedCharacterManager::SetRunnerInputLocked(bool bLocked)
{
	if (!HasAuthority())
	{
		return;
	}

	bRunnerInputLocked = bLocked;
}

// [한길] 0712
// true로 설정하면 역할 배정과 상관없이 이동, 점프, 슬라이드, 발사, 카메라 회전 요청을 모두 통과시킵니다.
void ASharedCharacterManager::SetIgnorePlayerRole(bool bShouldIgnorePlayerRole)
{
	if (!HasAuthority())
	{
		return;
	}

	bIgnorePlayerRole = bShouldIgnorePlayerRole;

	UE_LOG(
		LogCrazyRoo,
		Warning,
		TEXT("Ignore Player Role: %s"),
		bIgnorePlayerRole ? TEXT("true") : TEXT("false")
	);
}

// 적이 스폰될 때마다 호출 → 그 적의 공격 이벤트를 구독
void ASharedCharacterManager::HandleEnemySpawned(AFlyingEnemySpawner* Spawner, AFlyingEnemy* SpawnedEnemy)
{
	if (IsValid(SpawnedEnemy))
	{
		SpawnedEnemy->OnFlyingEnemyAttackHitPlayer.AddDynamic(this, &ASharedCharacterManager::HandleEnemyAttackHitPlayer);
	}
}

// 적의 주기 공격이 명중했을 때 → 공유 캐릭터 HP 차감
void ASharedCharacterManager::HandleEnemyAttackHitPlayer(AFlyingEnemy* Enemy, AActor* TargetPlayer, float Damage)
{
	if (SharedCharacter)
	{
		// 총 쏘는 공격은 무적 상태를 무시하고 무조건 데미지를 줍니다.
		SharedCharacter->ApplyPlayerDamage(Damage, Enemy, true);
	}
}

void ASharedCharacterManager::HandleObstacleSpawned(ARunnerObstacleSpawner* Spawner, ARunnerObstacle* SpawnedObstacle)
{
	if (IsValid(SpawnedObstacle))
	{
		SpawnedObstacle->OnPlayerHitObstacle.AddDynamic(this, &ASharedCharacterManager::HandlePlayerHitObstacle);
	}
}

void ASharedCharacterManager::HandlePlayerHitObstacle(ARunnerObstacle* Obstacle, AActor* PlayerActor,
	ERunnerObstacleActionType ActionType, ERunnerLaneCoverage LaneCoverage, ERunnerLane Lane)
{
	// [디버그 추가] 매니저가 이벤트를 제대로 받았는지 확인
	UE_LOG(LogTemp, Warning, TEXT("[MANAGER] 장애물 충돌 이벤트 수신 완료!"));

	// 들어온 PlayerActor가 현재 조작중인 공유 캐릭터가 맞는지 확인
	if (SharedCharacter && SharedCharacter == PlayerActor)
	{
		// [한길] 0713
		// SlideRequired 장애물은 캡슐 크기를 줄이지 않고 슬라이딩 상태값으로 회피 판정합니다.
		// 이렇게 하면 슬라이딩 중 캡슐 높이 변경으로 생기던 떨림을 피할 수 있습니다.
		if (ActionType == ERunnerObstacleActionType::SlideRequired && SharedCharacter->IsSliding())
		{
			UE_LOG(LogTemp, Warning, TEXT("[MANAGER] SlideRequired 장애물 회피 성공"));
			return;
		}

		// 상속받은 여러가지 Obstacle bp에서 데미지를 수정 가능
		// [수정] 장애물 충돌은 기본값(false)이 들어가므로 1초 무적이 정상 적용됩니다.
		SharedCharacter->ApplyPlayerDamage(Obstacle->GetObstacleDamage(), Obstacle, false);
	}
}

// [한길] 0707 전/후진 변경.
void ASharedCharacterManager::SetSharedCharacterMoveMode(ERunnerMoveMode NewMoveMode)
{
	if (!HasAuthority() || !SharedCharacter)
	{
		return;
	}

	SharedCharacter->SetMoveMode(NewMoveMode);
}

void ASharedCharacterManager::StartAutoMove()
{
	SetSharedCharacterMoveMode(ERunnerMoveMode::AutoMove);
}

void ASharedCharacterManager::StartManualMove()
{
	SetSharedCharacterMoveMode(ERunnerMoveMode::ManualMove);
}

void ASharedCharacterManager::StopMove()
{
	SetSharedCharacterMoveMode(ERunnerMoveMode::Stop);
}

void ASharedCharacterManager::StartAutoMoveAfterDelay()
{
	// 자동 이동 시작 여부는 서버가 결정합니다.
	if (!HasAuthority() || !SharedCharacter)
	{
		return;
	}

	// 시작 전에 캐릭터를 확실히 정지 상태로 설정합니다.
	SharedCharacter->SetMoveMode(ERunnerMoveMode::Stop);

	// 이전 타이머가 남아 있다면 제거하여 중복 실행을 방지합니다.
	GetWorldTimerManager().ClearTimer(AutoMoveStartTimerHandle);

	// AutoMoveStartDelay초 뒤 HandleAutoMoveStart를 한 번 호출합니다.
	GetWorldTimerManager().SetTimer(
		AutoMoveStartTimerHandle,
		this,
		&ASharedCharacterManager::HandleAutoMoveStart,
		AutoMoveStartDelay,
		false
	);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("AutoMove will start after %.1f seconds."),
		AutoMoveStartDelay
	);
}

void ASharedCharacterManager::HandleAutoMoveStart()
{
	if (!HasAuthority() || !SharedCharacter)
	{
		return;
	}

	// 대기 시간이 끝났으므로 자동 이동을 시작합니다.
	SharedCharacter->SetMoveMode(ERunnerMoveMode::AutoMove);

	UE_LOG(LogTemp, Warning, TEXT("AutoMove Started"));
}

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
}

void ASharedCharacterManager::RequestMoveLeft(ECrazyRooPlayerRole RequestRole)
{
	if (!HasAuthority() || !SharedCharacter)
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
	if (!HasAuthority() || !SharedCharacter)
	{
		return;
	}


	if (!bIgnorePlayerRole && RequestRole != ECrazyRooPlayerRole::P_Right)
	{
		UE_LOG(LogCrazyRoo, Warning, TEXT("MoveRight 무시: 요청역할 %s (P_Right 아님)"),CrazyRooRoleToString(RequestRole));
		return;
	}

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
	if (!HasAuthority() || !SharedCharacter)
	{
		return;
	}


	if (!bIgnorePlayerRole && RequestRole != ECrazyRooPlayerRole::P_Acrobat)
	{
		UE_LOG(LogCrazyRoo, Warning, TEXT("Jump 무시: 요청역할 %s (P_Acrobat 아님)"),CrazyRooRoleToString(RequestRole));
		return;
	}

	SharedCharacter->DoJump();
}

void ASharedCharacterManager::RequestSlide(ECrazyRooPlayerRole RequestRole)
{
	if (!HasAuthority() || !SharedCharacter)
	{
		return;
	}

	if (!bIgnorePlayerRole && RequestRole != ECrazyRooPlayerRole::P_Acrobat)
	{
		UE_LOG(LogCrazyRoo, Warning, TEXT("Slide 무시: 요청역할 %s (P_Acrobat 아님)"),CrazyRooRoleToString(RequestRole));
		return;
	}

	SharedCharacter->DoSlide();
}

void ASharedCharacterManager::RequestFireToLocation(ECrazyRooPlayerRole RequestRole, const FVector& TargetLocation)
{
	if (!HasAuthority() || !SharedCharacter)
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
	if (!HasAuthority() || !SharedCharacter)
	{
		return;
	}

	if (!bIgnorePlayerRole && RequestRole != ECrazyRooPlayerRole::P_Shooter)
	{
		UE_LOG(LogCrazyRoo, Warning, TEXT("CameraLeft 무시: 요청역할 %s (P_Shooter 아님)"),CrazyRooRoleToString(RequestRole));
		return;
	}

	SharedCharacter->RotateCameraLeft();
}

void ASharedCharacterManager::RequestRotateCameraRight(ECrazyRooPlayerRole RequestRole)
{
	if (!HasAuthority() || !SharedCharacter)
	{
		return;
	}

	if (!bIgnorePlayerRole && RequestRole != ECrazyRooPlayerRole::P_Shooter)
	{
		UE_LOG(LogCrazyRoo, Warning, TEXT("CameraRight 무시: 요청역할 %s (P_Shooter 아님)"),CrazyRooRoleToString(RequestRole));
		return;
	}

	SharedCharacter->RotateCameraRight();
}

void ASharedCharacterManager::RequestFireAimRay(ECrazyRooPlayerRole RequestRole, const FVector& AimOrigin, const FVector& AimDirection)
{
	if (!HasAuthority() || !SharedCharacter)
	{
		return;
	}

	if (!bIgnorePlayerRole && RequestRole != ECrazyRooPlayerRole::P_Shooter)
	{
		UE_LOG(LogCrazyRoo, Warning, TEXT("FireAimRay 무시: 요청역할 %s (P_Shooter 아님)"),CrazyRooRoleToString(RequestRole));
		return;
	}

	SharedCharacter->FireByAimRay(AimOrigin, AimDirection);
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

		StopFlyingEnemiesForRunnerTurn();

		// 캐릭터를 회전시킵니다.
		SharedCharacter->ApplyCornerTurn(YawDelta);

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
	if (!HasAuthority() || !SharedCharacter)
	{
		return;
	}

	SharedCharacter->DebugMoveForward(AxisValue);
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
		SharedCharacter->ApplyPlayerDamage(Damage, Enemy);
	}
}

// SharedCharacterManager.cpp

#include "LHG/Manager/SharedCharacterManager.h"
#include "LHG/Character/SharedRailCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

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

void ASharedCharacterManager::RequestMoveLeft(ERailPlayerRole RequestRole)
{
	if (!HasAuthority() || !SharedCharacter)
    {
		return;
    }

	// bIgnorePlayerRole이 false일 때만 역할 검사
	if (!bIgnorePlayerRole &&
		RequestRole != ERailPlayerRole::LeftControl)
	{
		return;
	}

	SharedCharacter->MoveLeft();
}

void ASharedCharacterManager::RequestMoveRight(ERailPlayerRole RequestRole)
{
	if (!HasAuthority() || !SharedCharacter)
    {
		return;
    }

	if (!bIgnorePlayerRole &&
	 RequestRole != ERailPlayerRole::RightControl)
	{
		return;
	}

	SharedCharacter->MoveRight();
}

void ASharedCharacterManager::RequestJump(ERailPlayerRole RequestRole)
{
	if (!HasAuthority() || !SharedCharacter)
    {
		return;
    }

	if (!bIgnorePlayerRole &&
	  RequestRole != ERailPlayerRole::JumpSlideControl)
	{
		return;
	}

	SharedCharacter->DoJump();
}

void ASharedCharacterManager::RequestSlide(ERailPlayerRole RequestRole)
{
	if (!HasAuthority() || !SharedCharacter)
    {
		return;
    }

	if (!bIgnorePlayerRole &&
		RequestRole != ERailPlayerRole::JumpSlideControl)
	{
		return;
	}

	SharedCharacter->DoSlide();
}

void ASharedCharacterManager::RequestFireToLocation(
	ERailPlayerRole RequestRole,
	const FVector& TargetLocation
)
{
	if (!HasAuthority() || !SharedCharacter)
	{
		return;
	}

	if (!bIgnorePlayerRole &&
		RequestRole != ERailPlayerRole::AimShootControl)
	{
		return;
	}

	SharedCharacter->FireToLocation(TargetLocation);
}

void ASharedCharacterManager::RequestRotateCameraLeft(ERailPlayerRole RequestRole)
{
	if (!HasAuthority() || !SharedCharacter)
	{
		return;
	}

	// 테스트 모드가 아니면 AimShootControl만 카메라 회전 가능
	if (!bIgnorePlayerRole &&
		RequestRole != ERailPlayerRole::AimShootControl)
	{
		return;
	}

	SharedCharacter->RotateCameraLeft();
}

void ASharedCharacterManager::RequestRotateCameraRight(ERailPlayerRole RequestRole)
{
	if (!HasAuthority() || !SharedCharacter)
	{
		return;
	}

	// 테스트 모드가 아니면 AimShootControl만 카메라 회전 가능
	if (!bIgnorePlayerRole &&
		RequestRole != ERailPlayerRole::AimShootControl)
	{
		return;
	}

	SharedCharacter->RotateCameraRight();
}

void ASharedCharacterManager::RequestFireAimRay(
	ERailPlayerRole RequestRole,
	const FVector& AimOrigin,
	const FVector& AimDirection
)
{
	if (!HasAuthority() || !SharedCharacter)
	{
		return;
	}

	if (!bIgnorePlayerRole &&
		RequestRole != ERailPlayerRole::AimShootControl)
	{
		return;
	}

	SharedCharacter->FireByAimRay(AimOrigin, AimDirection);
}

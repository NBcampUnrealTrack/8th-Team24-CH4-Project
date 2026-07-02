// SharedCharacterManager.cpp

#include "LHG/Manager/SharedCharacterManager.h"
#include "LHG/Character/SharedRailCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "CrazyRooTypes.h"
#include "CrazyRoo.h"

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

// SharedRailCharacter.cpp

#include "Player/SharedRailCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

ASharedRailCharacter::ASharedRailCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // 이 캐릭터가 네트워크에서 복제되도록 설정
    bReplicates = true;

    // 이동도 클라이언트들에게 복제
    SetReplicateMovement(true);

    // 시작 레일은 중앙
    CurrentLane = 1;

    // 카메라 암 생성
    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(RootComponent);

    // 쿼터뷰용 거리와 각도
    SpringArm->TargetArmLength = 800.f;
    SpringArm->SetRelativeRotation(FRotator(-45.f, 0.f, 0.f));
    SpringArm->bUsePawnControlRotation = false;

    // 카메라 생성
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(SpringArm);

    FollowCamera->bUsePawnControlRotation = false;
}

void ASharedRailCharacter::BeginPlay()
{
    Super::BeginPlay();

    StartLocation = GetActorLocation();
	LaneCenterLocation = GetActorLocation();

    if (!SpringArm)
    {
        UE_LOG(LogTemp, Error, TEXT("SpringArm is NULL in BeginPlay"));
        return;
    }

    // 시작 카메라 방향 저장
    TargetCameraYaw = SpringArm->GetRelativeRotation().Yaw;

    UE_LOG(LogTemp, Warning, TEXT("SharedRailCharacter BeginPlay"));

    UE_LOG(LogTemp, Warning, TEXT("StartLocation: %s"), *StartLocation.ToString());
}

void ASharedRailCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 서버에서만 실제 위치 계산
    // 클라이언트는 서버에서 복제된 위치를 받음
    if (!HasAuthority())
    {
        return;
    }

    FVector CurrentLocation = GetActorLocation();
    FVector TargetLocation = GetTargetLaneLocation();

    // 현재 위치에서 목표 레일까지 부드럽게 이동
    FVector NewLocation = FMath::VInterpTo(
        CurrentLocation,
        TargetLocation,
        DeltaTime,
        LaneInterpSpeed
    );

    SetActorLocation(NewLocation);

    // 카메라 회전은 서버/클라이언트 모두에서 보간
    FRotator CurrentRot = SpringArm->GetRelativeRotation();
    FRotator TargetRot = FRotator(-45.f, TargetCameraYaw, 0.f);

    FRotator NewRot = FMath::RInterpTo(
        CurrentRot,
        TargetRot,
        DeltaTime,
        CameraInterpSpeed
        );

    SpringArm->SetRelativeRotation(NewRot);
}

FVector ASharedRailCharacter::GetTargetLaneLocation() const
{
	// 캐릭터 기준 오른쪽 방향
	FVector RightVector = GetActorRightVector();

	// 0 = 왼쪽, 1 = 중앙, 2 = 오른쪽
	float LaneOffset = (CurrentLane - 1) * LaneDistance;

	// 중앙 레일 기준 위치 + 캐릭터 오른쪽 방향 * 레일 간격
	return LaneCenterLocation + RightVector * LaneOffset;
}

void ASharedRailCharacter::MoveLeft()
{
    // 공유 캐릭터 조작은 서버에서.
    if (!HasAuthority())
    {
        return;
    }

    CurrentLane = FMath::Clamp(CurrentLane-1,0,2);

    UE_LOG(LogTemp,Warning,TEXT("MoveLeft / CurrentLane: %d"),CurrentLane);
}

void ASharedRailCharacter::MoveRight()
{
    if (!HasAuthority())
    {
        return;
    }

    CurrentLane = FMath::Clamp(CurrentLane+1, 0, 2);

    UE_LOG(LogTemp, Warning, TEXT("MoveRight / CurrentLane: %d"), CurrentLane);
}

void ASharedRailCharacter::DoJump()
{
    if (!HasAuthority())
    {
        return;
    }

    Jump();
    UE_LOG(LogTemp, Warning, TEXT("Jump"));
}

void ASharedRailCharacter::DoSlide()
{
    if (!HasAuthority())
    {
        return;
    }

    if (bIsSliding)
    {
        return;
    }

    bIsSliding = true;

    // 슬라이딩 중에는 캡슐 높이를 낮춤
    GetCapsuleComponent()->SetCapsuleHalfHeight(44.f);

    // 0.8초 후 슬라이딩 종료
    GetWorldTimerManager().SetTimer(
        SlideTimerHandle,
        this,
        &ASharedRailCharacter::EndSlide,
        0.8f,
        false
    );

    UE_LOG(LogTemp,Warning,TEXT("SharedCharacter Slide"));
}

void ASharedRailCharacter::RotateCameraLeft()
{
    if (!HasAuthority())
    {
        return;
    }

    // Q: 좌측으로 90도 회전
    TargetCameraYaw -= 90.f;

    UE_LOG(LogTemp, Warning, TEXT("Rotate Camera Left / TargetYaw: %f"), TargetCameraYaw);
}

void ASharedRailCharacter::RotateCameraRight()
{
    if (!HasAuthority())
    {
        return;
    }

    // E: 우측으로 90도 회전
    TargetCameraYaw += 90.f;

    UE_LOG(LogTemp, Warning, TEXT("Rotate Camera Right / TargetYaw: %f"), TargetCameraYaw);
}

void ASharedRailCharacter::EndSlide()
{
    if (!HasAuthority())
    {
        return;
    }

    bIsSliding = false;

    // 캡슐 높이 원래대로 복구
    GetCapsuleComponent()->SetCapsuleHalfHeight(88.f);

    UE_LOG(LogTemp,Warning,TEXT("SharedCharacter EndSlide"));
}

void ASharedRailCharacter::FireToLocation(const FVector& TargetLocation)
{
    if (!HasAuthority())
    {
        return;
    }

    FVector Start = GetActorLocation();

    // 캐릭터 가슴 높이 정도에서 발사
    Start.Z += 80.f;

    FVector Direction = TargetLocation - Start;
    Direction.Z = 0.f; // 쿼터뷰이므로 수평 방향으로만 발사
    Direction.Normalize();

    FVector End = Start + Direction * FireRange;

    FHitResult Hit;

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    bool bHit = GetWorld()->LineTraceSingleByChannel(
        Hit,
        Start,
        End,
        ECC_Visibility,
        Params
    );

    DrawDebugLine(
        GetWorld(),
        Start,
        End,
        bHit ? FColor::Red : FColor::Green,
        false,
        1.0f,
        0,
        3.f
    );

    if (bHit)
    {
        AActor* HitActor = Hit.GetActor();

        UE_LOG(LogTemp, Warning, TEXT("Fire Hit: %s"),
            HitActor ? *HitActor->GetName() : TEXT("None"));

        if (HitActor)
        {
            UGameplayStatics::ApplyDamage(
                HitActor,
                FireDamage,
                nullptr,
                this,
                nullptr
            );
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Fire Miss"));
    }
}

void ASharedRailCharacter::FireByAimRay(const FVector& AimOrigin, const FVector& AimDirection)
{
    if (!HasAuthority())
    {
        return;
    }

    FVector NormalizedAimDirection = AimDirection.GetSafeNormal();

    FVector AimEnd = AimOrigin + NormalizedAimDirection * AimRayDistance;

    FHitResult AimHit;

    FCollisionQueryParams AimParams;
    AimParams.AddIgnoredActor(this);

    bool bAimHit = GetWorld()->LineTraceSingleByChannel(
        AimHit,
        AimOrigin,
        AimEnd,
        ECC_Visibility,
        AimParams
    );

    // 마우스 커서가 실제로 가리킨 월드 지점
    FVector TargetPoint = bAimHit ? AimHit.ImpactPoint : AimEnd;

    // 총구 위치
    FVector MuzzleLocation = GetActorLocation();
    MuzzleLocation.Z += MuzzleHeight;

    // 총구에서 조준 지점으로 발사
    FVector ShotDirection = (TargetPoint - MuzzleLocation).GetSafeNormal();
    FVector ShotEnd = MuzzleLocation + ShotDirection * FireRange;

    FHitResult ShotHit;

    FCollisionQueryParams ShotParams;
    ShotParams.AddIgnoredActor(this);

    bool bShotHit = GetWorld()->LineTraceSingleByChannel(
        ShotHit,
        MuzzleLocation,
        ShotEnd,
        ECC_Visibility,
        ShotParams
    );

    DrawDebugLine(
        GetWorld(),
        MuzzleLocation,
        ShotEnd,
        bShotHit ? FColor::Red : FColor::Green,
        false,
        1.0f,
        0,
        3.f
    );

    if (bShotHit)
    {
        AActor* HitActor = ShotHit.GetActor();

        UE_LOG(LogTemp, Warning, TEXT("Shot Hit: %s"),
            HitActor ? *HitActor->GetName() : TEXT("None"));

        if (HitActor)
        {
            UGameplayStatics::ApplyDamage(
                HitActor,
                FireDamage,
                nullptr,
                this,
                nullptr
            );
        }
    }
}

void ASharedRailCharacter::UpdateLaneCenter()
{
	if (!HasAuthority())
	{
		return;
	}

	// 현재 위치를 새로운 중앙 레일 기준점으로 저장
	LaneCenterLocation = GetActorLocation();

	// 회전 후에는 중앙 레일로 초기화하는 것이 안전함
	CurrentLane = 1;

	UE_LOG(LogTemp, Warning, TEXT("LaneCenter Updated: %s"), *LaneCenterLocation.ToString());
}

void ASharedRailCharacter::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps
) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ASharedRailCharacter, CurrentLane);
    DOREPLIFETIME(ASharedRailCharacter, bIsSliding);
    DOREPLIFETIME(ASharedRailCharacter, TargetCameraYaw);
	DOREPLIFETIME(ASharedRailCharacter, LaneCenterLocation);
}

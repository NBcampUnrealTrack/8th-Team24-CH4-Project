// SharedRailCharacter.cpp

#include "Player/SharedRailCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Stage/RunnerDeathSequence.h"
#include "Weapons/RailProjectile.h"
#include "CrazyRoo.h"

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
    SpringArm->SetRelativeRotation(FRotator(CameraPitch, 0.f, 0.f));
    SpringArm->bUsePawnControlRotation = false;

    // 카메라 생성
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(SpringArm);

	FollowCamera->bUsePawnControlRotation = false;

	Tags.Add(TEXT("Player"));//스포너에 충돌과 피격판정을 태그로 검사해서 추가

	// QTE 몽타주 안에서 순서대로 재생할 기본 펀치 Section 이름입니다.
	QTEPunchComboSections =
	{
		TEXT("Punch_L_1"),
		TEXT("Punch_R_1"),
		TEXT("Punch_L_2"),
		TEXT("Punch_R_2")
	};
}

void ASharedRailCharacter::BeginPlay()
{
    Super::BeginPlay();

	// [한길] 0703 코너링 시스템.
    // TurnJudgeBox에서 플레이어를 인식하기 위한 태그입니다.
    // SharedCharacter는 Possess되지 않을 수도 있기 때문에
    // Player 태그를 추가하여 IsPlayerActor() 조건을 만족시킵니다.
    //Tags.AddUnique(TEXT("Player")); //생성자에 태그를 이미 추가해서 이 부분은 주석처리하겠습니다! [무송]

    StartLocation = GetActorLocation();
	LaneCenterLocation = GetActorLocation();

	// 서버에서 초기 체력을 설정하고 클라이언트로 동기화 시킵니다
	if (HasAuthority())
	{
		CurrentPlayerHP = MaxPlayerHP;
	}

    if (!SpringArm)
    {
        UE_LOG(LogTemp, Error, TEXT("SpringArm is NULL in BeginPlay"));
        return;
    }

    // 시작 카메라 방향 저장
    TargetCameraYaw = SpringArm->GetRelativeRotation().Yaw;

    UE_LOG(LogTemp, Warning, TEXT("SharedRailCharacter BeginPlay"));

    UE_LOG(LogTemp, Warning, TEXT("StartLocation: %s"), *StartLocation.ToString());

	UE_LOG(LogTemp, Warning, TEXT("CameraPitch: %f"), CameraPitch);
}

void ASharedRailCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

	// [한길] 0712
	// 카메라 회전은 위치 이동과 달리 로컬에서 보이는 SpringArm 회전을 갱신해야 합니다.
	// TargetCameraYaw는 서버에서 복제되고, 서버/클라이언트 모두 Tick에서 보간해야 실제 화면에서도 회전이 보입니다.
	if (SpringArm)
	{
		FRotator CurrentRot = SpringArm->GetRelativeRotation();
		FRotator TargetRot = FRotator(CameraPitch, TargetCameraYaw, 0.f);

		FRotator NewRot = FMath::RInterpTo(
			CurrentRot,
			TargetRot,
			DeltaTime,
			CameraInterpSpeed
		);

		SpringArm->SetRelativeRotation(NewRot);
	}

    // 서버에서만 실제 위치 계산
    // 클라이언트는 서버에서 복제된 위치를 받음
    if (!HasAuthority())
    {
        return;
    }

	if (bIsCornerSnapInterpolating)
	{
		CornerSnapElapsedTime += DeltaTime;
		const float Alpha = CornerSnapDuration > 0.0f ? FMath::Clamp(CornerSnapElapsedTime / CornerSnapDuration, 0.0f, 1.0f) : 1.0f;
		const FVector NewLocation = FMath::Lerp(CornerSnapStartLocation, CornerSnapTargetLocation, Alpha);
		SetActorLocation(NewLocation, false);

		if (Alpha >= 1.0f)
		{
			FinishCornerSnapTurn();
		}
		return;
	}

	// [한길] 0707 이동 모드에 따라 전진 방식 결정
	switch (MoveMode)
	{
    case ERunnerMoveMode::Stop:
		// 정지 상태. 자동 이동 없음.
		break;

    case ERunnerMoveMode::AutoMove:
    	{
    		// 자동 전진
    		FVector MoveDelta =
				GetActorForwardVector() * AutoForwardMoveSpeed * DeltaTime;

    		AddActorWorldOffset(MoveDelta, true);

    		// 레일 기준점도 같이 이동해야 원래 위치로 끌려가지 않습니다.
    		LaneCenterLocation += MoveDelta;
    		break;
    	}

    case ERunnerMoveMode::ManualMove:
		// 수동 모드에서는 W/S 입력에서만 이동합니다.
		break;
	}

    FVector CurrentLocation = GetActorLocation();
    FVector TargetLocation = GetTargetLaneLocation();

	// 레일 보정은 좌우 위치만 맞추고, 점프/낙하 높이는 CharacterMovement가 관리하게 둡니다.
	// TargetLocation의 Z를 그대로 쓰면 점프 중에도 캡슐이 바닥 레일 높이로 다시 끌려갑니다.
	TargetLocation.Z = CurrentLocation.Z;

    // 현재 위치에서 목표 레일까지 부드럽게 이동
    FVector NewLocation = FMath::VInterpTo(
        CurrentLocation,
        TargetLocation,
        DeltaTime,
        LaneInterpSpeed
    );

    SetActorLocation(NewLocation);

	if (bDebugLaneCenterTracking)
	{
		DebugLaneCenterTrackingLogElapsed += DeltaTime;
		if (DebugLaneCenterTrackingLogElapsed >= 1.0f)
		{
			DebugLaneCenterTrackingLogElapsed = 0.0f;
			UE_LOG(LogTemp, Warning, TEXT("[FlyingEnemyTracking][SharedRail] Actor=%s ActorLocation=%s LaneCenterLocation=%s ActorForwardVector=%s CurrentLane=%d MoveMode=%d"),
				*GetName(),
				*GetActorLocation().ToString(),
				*LaneCenterLocation.ToString(),
				*GetActorForwardVector().ToString(),
				CurrentLane,
				static_cast<int32>(MoveMode));
		}
	}
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

	// 사망 후 입력 차단
	if (bPlayerDeathHandled)
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

	// 사망 후 입력 차단
	if (bPlayerDeathHandled)
	{
		return;
	}

    CurrentLane = FMath::Clamp(CurrentLane+1, 0, 2);

    UE_LOG(LogTemp, Warning, TEXT("MoveRight / CurrentLane: %d"), CurrentLane);
}

void ASharedRailCharacter::MoveToCenterLane()
{
	if (!HasAuthority())
	{
		return;
	}

	// 사망 후 입력 차단
	if (bPlayerDeathHandled)
	{
		return;
	}


	CurrentLane = 1;
	UE_LOG(LogTemp, Warning, TEXT("MoveToCenterLane / CurrentLane: %d"), CurrentLane);
}

void ASharedRailCharacter::StopMovementForQTE()
{
	if (!HasAuthority())
	{
		return;
	}

	StopJumping();
	MoveMode = ERunnerMoveMode::Stop;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->Velocity = FVector::ZeroVector;
	}

	UE_LOG(LogTemp, Warning, TEXT("StopMovementForQTE"));
}

void ASharedRailCharacter::DoJump()
{
    if (!HasAuthority())
    {
        return;
    }

	// 사망 후 입력 차단
	if (bPlayerDeathHandled)
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

	// 사망 후 입력 차단
	if (bPlayerDeathHandled)
	{
		return;
	}

    if (bIsSliding)
    {
        return;
    }

    bIsSliding = true;

	// [한길] 0713
	// 캡슐 높이를 직접 줄이면 레일 위치 보정/충돌 보정과 충돌해 캐릭터가 떨릴 수 있습니다.
	// 따라서 캡슐 높이를 낮추는 코드를 삭제하였습니다.
	// 슬라이딩은 bIsSliding 상태로만 판정하고, 시각 연출은 애니메이션이나 Mesh Offset으로 처리하면 좋을것 같습니다.

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

	// 사망 후 입력 차단
	if (bPlayerDeathHandled)
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

	// 사망 후 입력 차단
	if (bPlayerDeathHandled)
	{
		return;
	}

    // E: 우측으로 90도 회전
    TargetCameraYaw += 90.f;

    UE_LOG(LogTemp, Warning, TEXT("Rotate Camera Right / TargetYaw: %f"), TargetCameraYaw);
}

void ASharedRailCharacter::ResetCameraToForwardDirection()
{
	if (!HasAuthority())
	{
		return;
	}

	TargetCameraYaw = 0.f;

	UE_LOG(LogTemp, Warning, TEXT("Reset Camera To Forward Direction"));
}

void ASharedRailCharacter::EndSlide()
{
    if (!HasAuthority())
    {
        return;
    }

    bIsSliding = false;

	// [한길] 0713
	// DoSlide에서 캡슐 높이를 바꾸지 않으므로 복구할 캡슐 값도 없습니다.

    UE_LOG(LogTemp,Warning,TEXT("SharedCharacter EndSlide"));
}

void ASharedRailCharacter::FireToLocation(const FVector& TargetLocation)
{
    if (!HasAuthority())
    {
        return;
    }

	// 사망 후 입력 차단
	if (bPlayerDeathHandled)
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

	// 사망 후 입력 차단
	if (bPlayerDeathHandled)
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

void ASharedRailCharacter::ApplyCornerTurn(float YawDelta)
{
	if (!HasAuthority())
	{
		return;
	}

	// 캐릭터 회전 전에 카메라의 월드 회전을 저장합니다.
	FRotator SavedSpringArmWorldRotation=FRotator::ZeroRotator;//초기화 안해서 빌드오류가 생겨서 일단 0으로 초기화 선언하겠습니다 [무송]

	if (SpringArm)
	{
		SavedSpringArmWorldRotation = SpringArm->GetComponentRotation();
	}

	// 캐릭터만 회전시킵니다.
	FRotator NewRotation = GetActorRotation();
	NewRotation.Yaw += YawDelta;
	SetActorRotation(NewRotation);

	// 캐릭터가 회전하면서 SpringArm도 같이 돌았으므로,
	// 저장해둔 월드 회전으로 다시 되돌립니다.
	if (SpringArm)
	{
		SpringArm->SetWorldRotation(SavedSpringArmWorldRotation);

		// [한길] 0714 카메라 회전 오류 해결.
		// 보간 중인 SpringArm 현재값을 목표값으로 저장하지 않고,
		// 캐릭터 회전량만큼 목표 yaw를 보정해 카메라 월드 방향을 유지합니다.
		TargetCameraYaw = FRotator::NormalizeAxis(TargetCameraYaw - YawDelta);
	}

	UE_LOG(LogTemp, Warning, TEXT("Corner Turn Applied Character Only : %f"), YawDelta);
}

void ASharedRailCharacter::ApplyCornerTurnAtSnapPoint(float YawDelta, const FTransform& SnapTransform)
{
	if (!HasAuthority())
	{
		return;
	}

	FVector SnapLocation = SnapTransform.GetLocation();
	SnapLocation.Z = GetActorLocation().Z;
	PendingCornerYawDelta = YawDelta;
	CornerSnapTargetLocation = SnapLocation;

	if (CornerSnapDuration <= 0.0f || (GetActorLocation() - SnapLocation).IsNearlyZero(1.0f))
	{
		FinishCornerSnapTurn();
		return;
	}

	bIsCornerSnapInterpolating = true;
	CornerSnapElapsedTime = 0.0f;
	CornerSnapStartLocation = GetActorLocation();

	UE_LOG(LogTemp, Warning, TEXT("Corner Snap Interpolation Started : %f / TargetLocation: %s"), YawDelta, *SnapLocation.ToString());
}

void ASharedRailCharacter::FinishCornerSnapTurn()
{
	if (!HasAuthority())
	{
		return;
	}

	bIsCornerSnapInterpolating = false;
	SetActorLocation(CornerSnapTargetLocation, false);

	CurrentLane = 1;
	LaneCenterLocation = CornerSnapTargetLocation;

	FRotator SavedSpringArmWorldRotation = FRotator::ZeroRotator;
	if (SpringArm)
	{
		SavedSpringArmWorldRotation = SpringArm->GetComponentRotation();
	}

	FRotator NewRotation = GetActorRotation();
	NewRotation.Yaw += PendingCornerYawDelta;
	SetActorRotation(NewRotation);

	if (SpringArm)
	{
		SpringArm->SetWorldRotation(SavedSpringArmWorldRotation);
		// [한길] 0714 카메라 회전 오류 해결.
		TargetCameraYaw = FRotator::NormalizeAxis(TargetCameraYaw - PendingCornerYawDelta);
	}

	UE_LOG(LogTemp, Warning, TEXT("Corner Turn Applied At SnapPoint : %f / SnapLocation: %s"), PendingCornerYawDelta, *CornerSnapTargetLocation.ToString());

	PendingCornerYawDelta = 0.0f;
	CornerSnapElapsedTime = 0.0f;
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
	// [서희]최대 체력도 모든 클라이언트에 복제합니다.
	// 최대 목숨이 런타임 중 변경되어도 하트 개수가 갱신됩니다.
	DOREPLIFETIME(ASharedRailCharacter, MaxPlayerHP);
	DOREPLIFETIME(ASharedRailCharacter, CurrentPlayerHP);
	DOREPLIFETIME(ASharedRailCharacter, bIsInvincible); // 무적 상태 동기화 추가
	DOREPLIFETIME(ASharedRailCharacter, MoveMode); // 딜레이 스타트 위한 모드변경.
}

// ============================================================================
// [서희]체력 복제 및 UI 알림
// ============================================================================

void ASharedRailCharacter::OnRep_PlayerHP()
{
	// 이 함수는 클라이언트가 서버로부터
	// MaxPlayerHP 또는 CurrentPlayerHP의 새로운 값을 받았을 때 자동 호출됩니다.
	//
	// 데디케이티드 서버의 UI를 직접 건드리는 것이 아니라,
	// 각 클라이언트에서 자신의 WBP_InGameHUD에 체력 변경을 알립니다.
	BroadcastPlayerHP();
}

void ASharedRailCharacter::BroadcastPlayerHP()
{
	// 현재 체력과 최대 체력을 이벤트 구독자에게 전달합니다.
	//
	// WBP_InGameHUD가 OnPlayerHPChanged를 구독하고 있다면
	// HandlePlayerHPChanged(CurrentPlayerHP, MaxPlayerHP)가 실행됩니다.
	OnPlayerHPChanged.Broadcast(CurrentPlayerHP, MaxPlayerHP);
}

FVector ASharedRailCharacter::GetLaneCenterLocation() const
{
	return LaneCenterLocation;
}

FTransform ASharedRailCharacter::GetLaneCenterTransform() const
{
	const FRotator LaneCenterRotation(0.0f, GetActorRotation().Yaw, 0.0f);
	return FTransform(LaneCenterRotation, LaneCenterLocation);
}

void ASharedRailCharacter::ApplyPlayerDamage(float Damage, AActor* DamageCauser, bool bIgnoreInvincibility)
{
	//  예외 처리: 서버가 아니거나, 데미지가 없거나, 이미 죽은 상태면 로직을 무시
	if (!HasAuthority() || Damage <= 0.f || bPlayerDeathHandled)
	{
		return;
	}

	// 무적 상태라면 데미지 무시
	if (!bIgnoreInvincibility && bIsInvincible)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SERVER] 1초 무적 상태"));
		return;
	}

	// 체력 차감: FMath::Max를 사용해 체력이 음수로 내려가지 않도록 안전하게 계산
	CurrentPlayerHP = FMath::Max(0.f, CurrentPlayerHP - Damage);

	//[서희]추가
	BroadcastPlayerHP();

	// 피격 로그: 서버 디버깅을 위해 데미지 제공자와 남은 체력을 기록합니다.
	UE_LOG(LogTemp, Warning, TEXT("[SERVER] 플레이어 피격! -%.1f (원인: %s) → HP %.1f / %.1f"),
	   Damage,
	   DamageCauser ? *DamageCauser->GetName() : TEXT("Unknown"),
	   CurrentPlayerHP, MaxPlayerHP);

	// 사망 체크: 체력이 모두 소진되면 사망 로직을 호출합니다.
	if (CurrentPlayerHP <= 0.f)
	{
		HandlePlayerDeath();
	}
	else
	{
		// 일반 장애물일 때만 무적 타이머를 부여합니다.
		if (!bIgnoreInvincibility)
		{
			bIsInvincible = true;
			GetWorldTimerManager().SetTimer(
				InvincibilityTimerHandle,
				this,
				&ASharedRailCharacter::ResetInvincibility,
				InvincibilityDuration,
				false
			);
		}
	}
}

void ASharedRailCharacter::HandlePlayerDeath()
{
	bPlayerDeathHandled = true;   // 이후 피격/중복 사망 차단

	UE_LOG(LogTemp, Error, TEXT("[SERVER] 플레이어 사망"));
	Multicast_NotifyPlayerDeath();

	// 캐릭터 조작 및 이동 강제 정지
	SetMoveMode(ERunnerMoveMode::Stop);

	// 사망 후 캡슐 컴포넌트 제거 (선택사항)
	//if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	//{
	//	Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	//}

	//사망 애니메이션 재생 (멀티캐스트)
	Multicast_PlayDeathMontage();

	//UE_LOG(LogTemp, Log, TEXT("[SharedRailCharacter] Trigger RunnerDeathSequence"));
	//if (ARunnerDeathSequence* DeathSequence = ARunnerDeathSequence::FindDeathSequence(GetWorld()))
	//{
	//	DeathSequence->StartDeathSequence(this);
	//}
	//else
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("[SharedRailCharacter] RunnerDeathSequence not found"));
	//}

	// 애니메이션 길이에 맞춰 딜레이 설정
	float DeathDelay = 3.0f; // 딜레이 시간(몽타주가 없을시, 혹시모를 기본값)
	if (DeathMontage)
	{
		// 몽타주 애니메이션 길이에 0.5초를 더해 데드 시퀸스로 가기 전 죽은 모습을 보여줌
		DeathDelay = DeathMontage->GetPlayLength() + 0.5f;
	}

	UE_LOG(LogTemp, Log, TEXT("[SharedRailCharacter] %f초 후 RunnerDeathSequence 실행"), DeathDelay);

	// 타이머를 통해 지정된 시간 뒤에 TriggerDeathSequence 호출
	GetWorldTimerManager().SetTimer(
		DeathSequenceTimerHandle,
		this,
		&ASharedRailCharacter::TriggerDeathSequence,
		DeathDelay,
		false
	);
}

void ASharedRailCharacter::Multicast_NotifyPlayerDeath_Implementation()
{
	// 서버 + 모든 클라이언트 화면에서 실행됨
	FCrazyRooDebug::PrintSystem(this, TEXT("플레이어 사망!"), FColor::Red, 10.0f);
}

//[한길] 0703 테스트용 앞뒤 움직임.
void ASharedRailCharacter::DebugMoveForward(float AxisValue)
{
	if (!HasAuthority())
	{
		return;
	}

	// 0706 수동 모드가 아니면 W/S 입력 무시
	if (MoveMode != ERunnerMoveMode::ManualMove)
	{
		return;
	}

	if (FMath::IsNearlyZero(AxisValue))
	{
		return;
	}

	FVector MoveDelta =
		GetActorForwardVector() * AxisValue * DebugMoveSpeed * GetWorld()->GetDeltaSeconds();

	// 캐릭터 실제 위치 이동
	AddActorWorldOffset(MoveDelta, true);

	// 중요:
	// 레일 중앙 기준점도 같이 이동시켜야 Tick에서 원래 위치로 끌려가지 않음
	LaneCenterLocation += MoveDelta;
}

// [한길] 0707 움직임 방식 변경 함수.
void ASharedRailCharacter::SetMoveMode(ERunnerMoveMode NewMoveMode)
{
	if (!HasAuthority())
	{
		return;
	}

	MoveMode = NewMoveMode;

	UE_LOG(LogTemp, Warning, TEXT("MoveMode Changed: %d"), static_cast<uint8>(MoveMode));
}

void ASharedRailCharacter::Multicast_PlayPunchMontage_Implementation()
{
	// 멀티캐스트 구현부: 여기서 실제 몽타주 재생
	if (PunchMontage)
	{
		PlayAnimMontage(PunchMontage);
	}
}

void ASharedRailCharacter::Multicast_PlayQTEPunchMontage_Implementation()
{
	if (!QTEPunchMontage || QTEPunchComboSections.IsEmpty())
	{
		return;
	}

	// G키 입력 1회를 펀치 1회로 예약합니다. 몽타주가 이미 재생 중이면 다음 Notify에서 이어서 처리합니다.
	QueuedQTEPunchCount++;

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		ResetQTEPunchComboState();
		return;
	}

	if (!bIsPlayingQTEPunchCombo || !AnimInstance->Montage_IsPlaying(QTEPunchMontage))
	{
		PlayAnimMontage(QTEPunchMontage);
		bIsPlayingQTEPunchCombo = true;
		CurrentQTEPunchComboIndex = INDEX_NONE;
		PlayNextQTEPunchComboSection();
	}
}

void ASharedRailCharacter::Multicast_StopQTEPunchMontage_Implementation()
{
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (AnimInstance && QTEPunchMontage && AnimInstance->Montage_IsPlaying(QTEPunchMontage))
	{
		AnimInstance->Montage_Stop(0.15f, QTEPunchMontage);
	}

	ResetQTEPunchComboState();
}

void ASharedRailCharacter::Multicast_PlayDeathMontage_Implementation()
{
	// 모든 클라이언트 화면에서 사망 애니메이션 재생
	if (DeathMontage)
	{
		PlayAnimMontage(DeathMontage);
	}
}

void ASharedRailCharacter::TriggerDeathSequence()
{
	// 데스 시퀀스(철창 + 카메라 + UI) 최종 호출
	UE_LOG(LogTemp, Log, TEXT("[SharedRailCharacter] Trigger RunnerDeathSequence"));
	if (ARunnerDeathSequence* DeathSequence = ARunnerDeathSequence::FindDeathSequence(GetWorld()))
	{
		DeathSequence->StartDeathSequence(this);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[SharedRailCharacter] RunnerDeathSequence not found"));
	}
}

void ASharedRailCharacter::HandleQTEPunchComboCheck()
{
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance || !QTEPunchMontage || !AnimInstance->Montage_IsPlaying(QTEPunchMontage))
	{
		ResetQTEPunchComboState();
		return;
	}

	if (QueuedQTEPunchCount > 0)
	{
		// 예약된 G키 입력이 남아 있으면 다음 왼손/오른손 펀치 Section으로 이어갑니다.
		PlayNextQTEPunchComboSection();
		return;
	}

	// 더 이상 입력이 없으면 가드 복귀 Section으로 이동합니다.
	if (QTEPunchGuardEndSection != NAME_None)
	{
		AnimInstance->Montage_JumpToSection(QTEPunchGuardEndSection, QTEPunchMontage);
	}

	ResetQTEPunchComboState();
}

void ASharedRailCharacter::PlayNextQTEPunchComboSection()
{
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance || !QTEPunchMontage || QTEPunchComboSections.IsEmpty() || QueuedQTEPunchCount <= 0)
	{
		return;
	}

	QueuedQTEPunchCount--;

	// 마지막 펀치까지 간 뒤에도 입력이 계속 들어오면 다시 첫 번째 왼손 펀치부터 반복합니다.
	CurrentQTEPunchComboIndex = (CurrentQTEPunchComboIndex + 1) % QTEPunchComboSections.Num();
	const FName NextSectionName = QTEPunchComboSections[CurrentQTEPunchComboIndex];

	if (NextSectionName != NAME_None)
	{
		AnimInstance->Montage_JumpToSection(NextSectionName, QTEPunchMontage);
	}
}

void ASharedRailCharacter::ResetQTEPunchComboState()
{
	QueuedQTEPunchCount = 0;
	CurrentQTEPunchComboIndex = INDEX_NONE;
	bIsPlayingQTEPunchCombo = false;
}

void ASharedRailCharacter::ResetInvincibility()
{
	if (HasAuthority())
	{
		bIsInvincible = false;
		UE_LOG(LogTemp, Warning, TEXT("[SERVER] 무적 시간이 종료"));
	}
}

// [한길] 0710 발사방식 변경.
void ASharedRailCharacter::FireBySelectedMode(
	const FVector& AimOrigin,
	const FVector& AimDirection,
	AController* ShooterController
)
{
	if (!HasAuthority())
	{
		return;
	}

	// 사망 후 입력 차단
	if (bPlayerDeathHandled)
	{
		return;
	}

	// 모든 클라이언트 화면에서 펀치 애니메이션이 보이도록 호출
	Multicast_PlayPunchMontage();

	switch (FireMode)
	{
	case ERunnerFireMode::LineTrace:
		// 기존 팀 프로젝트의 LineTrace 방식
		FireByAimRay(AimOrigin, AimDirection);
		break;

	case ERunnerFireMode::Projectile:
		// 새로 추가한 Projectile 방식
		FireProjectileByAimRay(
			AimOrigin,
			AimDirection,
			ShooterController
		);
		break;

	default:
		break;
	}
}

// [한길] 0710 투사체 발사 방식.
void ASharedRailCharacter::FireProjectileByAimRay(
	const FVector& AimOrigin,
	const FVector& AimDirection,
	AController* ShooterController
)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!ProjectileClass)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("ProjectileClass is NULL")
		);
		return;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	const FVector NormalizedAimDirection =
		AimDirection.GetSafeNormal();

	/*
	 * 1단계:
	 * 카메라에서 마우스 커서 방향으로 LineTrace하여
	 * 실제 조준 지점을 찾습니다.
	 *
	 * 이 Trace는 데미지용이 아니라 조준 지점 계산용입니다.
	 */
	const FVector AimEnd =
		AimOrigin + NormalizedAimDirection * AimRayDistance;

	FHitResult AimHit;

	FCollisionQueryParams AimParams;
	AimParams.AddIgnoredActor(this);

	const bool bAimHit =
		World->LineTraceSingleByChannel(
			AimHit,
			AimOrigin,
			AimEnd,
			ECC_Visibility,
			AimParams
		);

	const FVector TargetPoint =
		bAimHit ? AimHit.ImpactPoint : AimEnd;

	/*
	 * 2단계:
	 * Projectile이 생성될 발사 위치를 계산합니다.
	 */
	FVector MuzzleLocation = GetActorLocation();

	MuzzleLocation +=
		GetActorForwardVector() * ProjectileSpawnForwardOffset;

	MuzzleLocation.Z += ProjectileSpawnHeight;

	/*
	 * 발사 위치에서 마우스 조준 지점을 바라보는 방향을 계산합니다.
	 */
	const FVector ShotDirection =
		(TargetPoint - MuzzleLocation).GetSafeNormal();

	const FRotator SpawnRotation =
		ShotDirection.Rotation();

	FActorSpawnParameters SpawnParameters;

	// 발사 캐릭터를 Owner로 설정
	SpawnParameters.Owner = this;

	// Character는 Pawn이므로 Instigator로 사용할 수 있습니다.
	SpawnParameters.Instigator = this;

	/*
	 * 발사 위치 주변에 약간의 충돌이 있어도 위치를 조정해서 스폰합니다.
	 * AlwaysSpawn보다 AdjustIfPossibleButAlwaysSpawn이 안전합니다.
	 */
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::
		AdjustIfPossibleButAlwaysSpawn;

	ARailProjectile* SpawnedProjectile =
		World->SpawnActor<ARailProjectile>(
			ProjectileClass,
			MuzzleLocation,
			SpawnRotation,
			SpawnParameters
		);

	if (!SpawnedProjectile)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Projectile Spawn Failed")
		);
		return;
	}

	SpawnedProjectile->InitializeProjectile(
		ShotDirection,
		ProjectileDamage,
		ShooterController
	);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Projectile Spawned: %s"),
		*MuzzleLocation.ToString()
	);

	if (bAimHit)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Aim Trace Hit: %s / Point: %s"),
			AimHit.GetActor() ? *AimHit.GetActor()->GetName() : TEXT("None"),
			*AimHit.ImpactPoint.ToString()
		);
	}
	else
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Aim Trace Miss / AimEnd: %s"),
			*AimEnd.ToString()
		);
	}

	DrawDebugLine(
	GetWorld(),
	MuzzleLocation,
	AimEnd,
	FColor::Blue,
	false,
	2.f,
	0,
	2.f
	);
}

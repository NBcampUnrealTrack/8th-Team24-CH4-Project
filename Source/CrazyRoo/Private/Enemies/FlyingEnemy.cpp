// Copyright Epic Games, Inc. All Rights Reserved.

#include "Enemies/FlyingEnemy.h"

#include "Audio/CrazyRooAudioNetManager.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/Pawn.h"
#include "Player/SharedRailCharacter.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

AFlyingEnemy::AFlyingEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// ── 네트워크: 서버가 스폰하면 클라에 자동 복제 ──
	bReplicates = true;
	SetReplicateMovement(true);   // Tick의 위치 이동을 클라로 전파
	bAlwaysRelevant = true;       // 폰 없는 구조라 관련성 탈락 방지
	//

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	StaticMeshComponent->SetupAttachment(SceneRoot);
	StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComponent"));
	SkeletalMeshComponent->SetupAttachment(SceneRoot);
	SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	HitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HitBox"));
	HitBox->SetupAttachment(SceneRoot);
	HitBox->SetBoxExtent(FVector(80.0f, 80.0f, 80.0f));
	HitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HitBox->SetCollisionObjectType(ECC_WorldDynamic);
	HitBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	HitBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	HitBox->SetGenerateOverlapEvents(true);

	CurrentHP = MaxHP;
}

void AFlyingEnemy::BeginPlay()
{
	Super::BeginPlay();

	// 판정/타이머는 서버 전용. 클라의 적은 복제된 위치를 받아 보여주기만 한다
	if (!HasAuthority())
	{
		return;
	}

	CurrentHP = MaxHP;
	MoveDirection = MoveDirection.GetSafeNormal();
	RefreshCollisionResponses();
	HitBox->OnComponentBeginOverlap.AddDynamic(this, &AFlyingEnemy::HandleHitBoxBeginOverlap);

	if (bAutoDestroyAfterLifetime && Lifetime > 0.0f)
	{
		GetWorldTimerManager().SetTimer(LifetimeTimerHandle, this, &AFlyingEnemy::DestroyByLifetime, Lifetime, false);
	}
}

void AFlyingEnemy::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(LifetimeTimerHandle);
		GetWorldTimerManager().ClearTimer(AttackTimerHandle);
		GetWorldTimerManager().ClearTimer(DeferredDestroyTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void AFlyingEnemy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 이동 계산은 서버 전용
	if (!HasAuthority())
	{
		return;
	}

	if (!bMovementEnabled)
	{
		return;
	}

	if (bUseSlotMovement)
	{
		const FVector TargetLocation = GetCurrentSlotWorldLocation();
		if (bDebugEnemyTracking)
		{
			DebugEnemyTrackingLogElapsed += DeltaSeconds;
			if (DebugEnemyTrackingLogElapsed >= 1.0f)
			{
				DebugEnemyTrackingLogElapsed = 0.0f;

				const FVector ReferenceLocation = GetReferenceLocation();
				const ASharedRailCharacter* SharedRailCharacter = Cast<ASharedRailCharacter>(SlotReferenceActor.Get());
				const FVector LaneCenterLocation = SharedRailCharacter ? SharedRailCharacter->GetLaneCenterLocation() : FVector::ZeroVector;

				UE_LOG(LogTemp, Warning, TEXT("[FlyingEnemyTracking][Tick] Enemy=%s EnemyLocation=%s TargetLocation=%s ReferenceLocation=%s bSharedRail=%s LaneCenterLocation=%s bUseSlotMovement=%s bHasArrivedAtSlot=%s bFollowReferenceActor=%s bMovementEnabled=%s DistanceToTarget=%.2f"),
					*GetName(),
					*GetActorLocation().ToString(),
					*TargetLocation.ToString(),
					*ReferenceLocation.ToString(),
					SharedRailCharacter ? TEXT("true") : TEXT("false"),
					*LaneCenterLocation.ToString(),
					bUseSlotMovement ? TEXT("true") : TEXT("false"),
					bHasArrivedAtSlot ? TEXT("true") : TEXT("false"),
					bFollowReferenceActor ? TEXT("true") : TEXT("false"),
					bMovementEnabled ? TEXT("true") : TEXT("false"),
					FVector::Dist(GetActorLocation(), TargetLocation));
			}
		}

		if (!bHasArrivedAtSlot)
		{
			const FVector CurrentLocation = GetActorLocation();
			FVector NextLocation = TargetLocation;
			NextLocation.Z = FMath::FInterpConstantTo(CurrentLocation.Z, TargetLocation.Z, DeltaSeconds, DescendSpeed);
			SetActorLocation(NextLocation, true);

			const float ZDistanceToSlot = FMath::Abs(GetActorLocation().Z - TargetLocation.Z);
			if (!bAttackTimerStarted && ZDistanceToSlot <= SlotArrivalTolerance)
			{
				StartAttackDelay();
			}

			if (ZDistanceToSlot <= 1.0f)
			{
				SetActorLocation(TargetLocation, false);
				bHasArrivedAtSlot = true;
			}
		}
		else if (bFollowReferenceActor)
		{
			SetActorLocation(TargetLocation, false);
		}

		UpdateLookAtReference();
		return;
	}

	const FVector Direction = ResolveMoveDirection();
	if (!Direction.IsNearlyZero() && MoveSpeed > 0.0f)
	{
		AddActorWorldOffset(Direction * MoveSpeed * DeltaSeconds, true);
	}
}

void AFlyingEnemy::InitializeFlyingEnemy(const FVector& InMoveDirection, AActor* InTargetActor)
{
	if (!HasAuthority())
	{
		return;
	}

	SetMoveDirection(InMoveDirection);
	SetMovementTarget(InTargetActor);
	CurrentHP = MaxHP;
}

void AFlyingEnemy::ConfigureSlotMovement(USceneComponent* InReferenceComponent, AActor* InReferenceActor, EFlyingEnemySlot InAssignedSlot, const FVector& InSlotLocalOffset, float InDescendSpeed, float InArrivalTolerance, float InAttackDelayAfterArrive, float InAttackDamage, AActor* InAttackTargetActor, bool bInFollowReferenceActor)
{
	if (!HasAuthority())
	{
		return;
	}

	SlotReferenceComponent = InReferenceComponent;
	SlotReferenceActor = InReferenceActor;
	if (!SlotReferenceActor.IsValid() && SlotReferenceComponent.IsValid())
	{
		SlotReferenceActor = SlotReferenceComponent->GetOwner();
	}
	AssignedSlot = InAssignedSlot;
	SlotLocalOffset = InSlotLocalOffset;
	DescendSpeed = FMath::Max(0.0f, InDescendSpeed);
	SlotArrivalTolerance = FMath::Max(0.0f, InArrivalTolerance);
	AttackDelayAfterArrive = FMath::Max(0.0f, InAttackDelayAfterArrive);
	AttackDamage = FMath::Max(0.0f, InAttackDamage);
	AttackTargetActor = InAttackTargetActor;
	bFollowReferenceActor = bInFollowReferenceActor;
	FTransform ReferenceTransform;
	bHasInitialSlotTracking = GetReferenceTransform(ReferenceTransform);
	if (bHasInitialSlotTracking)
	{
		InitialReferenceLocation = ReferenceTransform.GetLocation();
		InitialReferenceForward = ReferenceTransform.GetRotation().GetForwardVector().GetSafeNormal();
		if (InitialReferenceForward.IsNearlyZero())
		{
			InitialReferenceForward = FVector::ForwardVector;
		}
		InitialSlotWorldLocation = ReferenceTransform.TransformPosition(SlotLocalOffset);
	}
	else
	{
		InitialReferenceLocation = GetActorLocation();
		InitialReferenceForward = GetActorForwardVector().GetSafeNormal();
		InitialSlotWorldLocation = GetActorLocation();
		UE_LOG(LogTemp, Warning, TEXT("FlyingEnemy '%s' has no valid slot reference. Slot tracking will stay at the spawn location."), *GetName());
	}
	bHasArrivedAtSlot = false;
	bUseSlotMovement = true;
	bMovementEnabled = true;
	bCanAttack = true;
	bAttackTimerStarted = false;
	DebugEnemyTrackingLogElapsed = 0.0f;
	if (bDebugEnemyTracking)
	{
		const ASharedRailCharacter* SharedRailCharacter = Cast<ASharedRailCharacter>(SlotReferenceActor.Get());
		UE_LOG(LogTemp, Warning, TEXT("[FlyingEnemyTracking][Configure] Enemy=%s SlotReferenceActor=%s SlotReferenceComponent=%s bSharedRail=%s bFollowReferenceActor=%s bHasInitialSlotTracking=%s InitialReferenceLocation=%s InitialReferenceForward=%s InitialSlotWorldLocation=%s SlotLocalOffset=%s AttackTargetActor=%s"),
			*GetName(),
			*GetNameSafe(SlotReferenceActor.Get()),
			*GetNameSafe(SlotReferenceComponent.Get()),
			SharedRailCharacter ? TEXT("true") : TEXT("false"),
			bFollowReferenceActor ? TEXT("true") : TEXT("false"),
			bHasInitialSlotTracking ? TEXT("true") : TEXT("false"),
			*InitialReferenceLocation.ToString(),
			*InitialReferenceForward.ToString(),
			*InitialSlotWorldLocation.ToString(),
			*SlotLocalOffset.ToString(),
			*GetNameSafe(AttackTargetActor.Get()));
	}
	UpdateLookAtReference();
}

void AFlyingEnemy::SetMovementTarget(AActor* InTargetActor)
{
	TargetActor = InTargetActor;
}

void AFlyingEnemy::SetMoveDirection(const FVector& InMoveDirection)
{
	MoveDirection = InMoveDirection.GetSafeNormal();
	if (MoveDirection.IsNearlyZero())
	{
		MoveDirection = FVector(-1.0f, 0.0f, 0.0f);
	}
}

void AFlyingEnemy::SetMoveMode(EFlyingEnemyMoveMode InMoveMode)
{
	MoveMode = InMoveMode;
}

void AFlyingEnemy::DisableEnemyInteraction()
{
	if (!HasAuthority())
	{
		return;
	}

	bCanAttack = false;
	bMovementEnabled = false;
	GetWorldTimerManager().ClearTimer(AttackTimerHandle);
	bAttackTimerStarted = false;

	if (HitBox)
	{
		HitBox->SetGenerateOverlapEvents(false);
		HitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AFlyingEnemy::StopFollowingReference()
{
	bFollowReferenceActor = false;
	bUseSlotMovement = false;
	TargetActor = nullptr;
	SlotReferenceActor = nullptr;
	SlotReferenceComponent = nullptr;
	bHasInitialSlotTracking = false;
}

void AFlyingEnemy::ScheduleDestroy(float Delay)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!GetWorld())
	{
		Destroy();
		return;
	}

	GetWorldTimerManager().ClearTimer(LifetimeTimerHandle);
	GetWorldTimerManager().ClearTimer(DeferredDestroyTimerHandle);

	if (Delay <= 0.0f)
	{
		DestroyByLifetime();
		return;
	}

	GetWorldTimerManager().SetTimer(DeferredDestroyTimerHandle, this, &AFlyingEnemy::DestroyByLifetime, Delay, false);
}

void AFlyingEnemy::SetDebugEnemyTracking(bool bInDebugEnemyTracking)
{
	bDebugEnemyTracking = bInDebugEnemyTracking;
	DebugEnemyTrackingLogElapsed = 0.0f;
}

bool AFlyingEnemy::IsPlayerActor(AActor* Actor) const
{
	if (!IsValid(Actor))
	{
		return false;
	}

	if (Actor->ActorHasTag(TEXT("Player"))) //태그로 플레이어 검색
	{
		return true;
	}

	const APawn* Pawn = Cast<APawn>(Actor);
	return Pawn && Pawn->IsPlayerControlled();
}

float AFlyingEnemy::TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (Damage <= 0.0f)
	{
		return 0.0f;
	}

	ApplyDamage(Damage, DamageCauser, GetActorLocation(), FVector::ZeroVector);
	return Damage;
}

void AFlyingEnemy::ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse)
{
	if (!HasAuthority())
	{
		return;
	}

	if (Damage <= 0.0f || CurrentHP <= 0.0f)
	{
		return;
	}

	CurrentHP = FMath::Max(0.0f, CurrentHP - Damage);
	OnFlyingEnemyDamaged.Broadcast(this, Damage, CurrentHP, DamageCauser);

	MulticastPlayHitEffect(DamageLocation);

	if (CurrentHP <= 0.0f)
	{
		const FVector HitSoundLocation = DamageLocation.ContainsNaN() ? GetActorLocation() : DamageLocation;
		UE_LOG(LogTemp, Log, TEXT("[FlyingEnemy] Request EnemyHit SFX"));
		if (ACrazyRooAudioNetManager* AudioNetManager = ACrazyRooAudioNetManager::FindAudioNetManager(GetWorld()))
		{
			AudioNetManager->RequestPlaySFXAtLocation(ECrazyRooSFXType::EnemyHit, HitSoundLocation);
		}

		HandleDeath();
	}
}

void AFlyingEnemy::HandleDeath()
{
	GetWorldTimerManager().ClearTimer(AttackTimerHandle);
	bAttackTimerStarted = false;
	BroadcastDestroyedEvent();
	Destroy();
}

void AFlyingEnemy::ApplyHealing(float Healing, AActor* Healer)
{
	if (Healing > 0.0f && CurrentHP > 0.0f)
	{
		CurrentHP = FMath::Min(MaxHP, CurrentHP + Healing);
	}
}

void AFlyingEnemy::NotifyDanger(const FVector& DangerLocation, AActor* DangerSource)
{
}

void AFlyingEnemy::HandleHitBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (IsPlayerActor(OtherActor))
	{
		OnFlyingEnemyHitPlayer.Broadcast(this, OtherActor);
	}
}

void AFlyingEnemy::DestroyByLifetime()
{
	BroadcastDestroyedEvent();
	Destroy();
}

void AFlyingEnemy::TriggerAttackHitPlayer()
{
	if (!bCanAttack || CurrentHP <= 0.0f)
	{
		return;
	}

	AActor* TargetPlayer = IsValid(AttackTargetActor) ? AttackTargetActor.Get() : SlotReferenceActor.Get();
	if (!IsValid(TargetPlayer))
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[FlyingEnemy] Request EnemyAttack SFX"));
	if (ACrazyRooAudioNetManager* AudioNetManager = ACrazyRooAudioNetManager::FindAudioNetManager(GetWorld()))
	{
		AudioNetManager->RequestPlaySFXAtLocation(ECrazyRooSFXType::EnemyAttack, GetActorLocation());
	}

	OnFlyingEnemyAttackHitPlayer.Broadcast(this, TargetPlayer, AttackDamage);
}

void AFlyingEnemy::BroadcastDestroyedEvent()
{
	if (bDestroyEventBroadcasted)
	{
		return;
	}

	bDestroyEventBroadcasted = true;
	OnFlyingEnemyDestroyed.Broadcast(this);
}

void AFlyingEnemy::RefreshCollisionResponses()
{
	if (!HitBox)
	{
		return;
	}

	HitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HitBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	HitBox->SetCollisionResponseToChannel(ECC_Visibility, bBlockVisibilityChannel ? ECR_Block : ECR_Ignore);
	HitBox->SetGenerateOverlapEvents(true);
	// [한길] 0713
	// FlyingEnemy는 투사체 피격 대상이므로 서버에서도 Projectile을 Block합니다.
	HitBox->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
}

void AFlyingEnemy::StartAttackDelay()
{
	if (!bCanAttack || !GetWorld())
	{
		return;
	}

	if (GetWorldTimerManager().IsTimerActive(AttackTimerHandle))
	{
		return;
	}

	bAttackTimerStarted = true;
	const float SafeAttackInterval = FMath::Max(0.1f, AttackDelayAfterArrive);
	GetWorldTimerManager().SetTimer(AttackTimerHandle, this, &AFlyingEnemy::TriggerAttackHitPlayer, SafeAttackInterval, true);
}

FVector AFlyingEnemy::GetCurrentSlotWorldLocation() const
{
	FTransform CurrentReferenceTransform;
	if (GetReferenceTransform(CurrentReferenceTransform))
	{
		return CurrentReferenceTransform.TransformPosition(SlotLocalOffset);
	}

	return bHasInitialSlotTracking ? InitialSlotWorldLocation : GetActorLocation();
}

FVector AFlyingEnemy::GetReferenceLocation() const
{
	FTransform ReferenceTransform;
	if (GetReferenceTransform(ReferenceTransform))
	{
		return ReferenceTransform.GetLocation();
	}

	return GetActorLocation();
}

FVector AFlyingEnemy::GetLookAtLocation() const
{
	if (IsValid(AttackTargetActor))
	{
		return AttackTargetActor->GetActorLocation();
	}

	return GetReferenceLocation();
}

bool AFlyingEnemy::GetReferenceTransform(FTransform& OutTransform) const
{
	if (SlotReferenceComponent.IsValid())
	{
		OutTransform = SlotReferenceComponent->GetComponentTransform();
		return true;
	}

	if (SlotReferenceActor.IsValid())
	{
		if (const ASharedRailCharacter* SharedRailCharacter = Cast<ASharedRailCharacter>(SlotReferenceActor.Get()))
		{
			OutTransform = SharedRailCharacter->GetLaneCenterTransform();
			return true;
		}

		OutTransform = SlotReferenceActor->GetActorTransform();
		return true;
	}

	return false;
}

void AFlyingEnemy::UpdateLookAtReference()
{
	const FVector DirectionToReference = GetLookAtLocation() - GetActorLocation();
	if (DirectionToReference.IsNearlyZero())
	{
		return;
	}

	FRotator LookAtRotation = DirectionToReference.Rotation();
	LookAtRotation.Pitch = 0.0f;
	LookAtRotation.Roll = 0.0f;
	SetActorRotation(LookAtRotation);
}

FVector AFlyingEnemy::ResolveMoveDirection() const
{
	if (MoveMode == EFlyingEnemyMoveMode::TowardTarget && IsValid(TargetActor))
	{
		return (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	}

	return MoveDirection.GetSafeNormal();
}

void AFlyingEnemy::MulticastPlayHitEffect_Implementation(FVector EffectLocation)
{
	if (HitEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			HitEffect,
			EffectLocation,
			GetActorRotation(),
			HitEffectScale
		);
	}

	// Hit SFX is routed through ACrazyRooAudioNetManager on the server to avoid duplicate playback.
}

void AFlyingEnemy::MulticastPlayAttackSound_Implementation(FVector SoundLocation)
{
	// Attack SFX is routed through ACrazyRooAudioNetManager on the server to avoid duplicate playback.
}

// Copyright Epic Games, Inc. All Rights Reserved.

#include "Enemies/FlyingEnemy.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

AFlyingEnemy::AFlyingEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

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

	if (!bMovementEnabled)
	{
		return;
	}

	if (bUseSlotMovement)
	{
		const FVector TargetLocation = GetCurrentSlotWorldLocation();
		if (!bHasArrivedAtSlot)
		{
			const FVector NextLocation = FMath::VInterpConstantTo(GetActorLocation(), TargetLocation, DeltaSeconds, DescendSpeed);
			SetActorLocation(NextLocation, true);

			if (FVector::DistSquared(GetActorLocation(), TargetLocation) <= FMath::Square(SlotArrivalTolerance))
			{
				SetActorLocation(TargetLocation, false);
				bHasArrivedAtSlot = true;
				StartAttackDelay();
			}
		}
		else if (bFollowReferenceActor)
		{
			SetActorLocation(TargetLocation, false);
		}

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
	SetMoveDirection(InMoveDirection);
	SetMovementTarget(InTargetActor);
	CurrentHP = MaxHP;
}

void AFlyingEnemy::ConfigureSlotMovement(USceneComponent* InReferenceComponent, AActor* InReferenceActor, EFlyingEnemySlot InAssignedSlot, const FVector& InSlotLocalOffset, float InDescendSpeed, float InArrivalTolerance, float InAttackDelayAfterArrive, float InAttackDamage, AActor* InAttackTargetActor, bool bInFollowReferenceActor)
{
	SlotReferenceComponent = InReferenceComponent;
	SlotReferenceActor = InReferenceActor;
	AssignedSlot = InAssignedSlot;
	SlotLocalOffset = InSlotLocalOffset;
	DescendSpeed = FMath::Max(0.0f, InDescendSpeed);
	SlotArrivalTolerance = FMath::Max(0.0f, InArrivalTolerance);
	AttackDelayAfterArrive = FMath::Max(0.0f, InAttackDelayAfterArrive);
	AttackDamage = FMath::Max(0.0f, InAttackDamage);
	AttackTargetActor = InAttackTargetActor;
	bFollowReferenceActor = bInFollowReferenceActor;
	bHasArrivedAtSlot = false;
	bUseSlotMovement = true;
	bMovementEnabled = true;
	bCanAttack = true;
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
	bCanAttack = false;
	bMovementEnabled = false;
	GetWorldTimerManager().ClearTimer(AttackTimerHandle);

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
}

void AFlyingEnemy::ScheduleDestroy(float Delay)
{
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

bool AFlyingEnemy::IsPlayerActor(AActor* Actor) const
{
	if (!IsValid(Actor))
	{
		return false;
	}

	if (Actor->ActorHasTag(TEXT("Player")))
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
	if (Damage <= 0.0f || CurrentHP <= 0.0f)
	{
		return;
	}

	CurrentHP = FMath::Max(0.0f, CurrentHP - Damage);
	OnFlyingEnemyDamaged.Broadcast(this, Damage, CurrentHP, DamageCauser);

	if (CurrentHP <= 0.0f)
	{
		HandleDeath();
	}
}

void AFlyingEnemy::HandleDeath()
{
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
	if (!bCanAttack)
	{
		return;
	}

	AActor* TargetPlayer = IsValid(AttackTargetActor) ? AttackTargetActor.Get() : SlotReferenceActor.Get();
	if (!IsValid(TargetPlayer))
	{
		return;
	}

	bCanAttack = false;
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
}

void AFlyingEnemy::StartAttackDelay()
{
	if (!bCanAttack || !GetWorld())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(AttackTimerHandle);

	if (AttackDelayAfterArrive <= 0.0f)
	{
		TriggerAttackHitPlayer();
		return;
	}

	GetWorldTimerManager().SetTimer(AttackTimerHandle, this, &AFlyingEnemy::TriggerAttackHitPlayer, AttackDelayAfterArrive, false);
}

FVector AFlyingEnemy::GetCurrentSlotWorldLocation() const
{
	if (SlotReferenceComponent.IsValid())
	{
		return SlotReferenceComponent->GetComponentTransform().TransformPosition(SlotLocalOffset);
	}

	if (SlotReferenceActor.IsValid())
	{
		return SlotReferenceActor->GetActorTransform().TransformPosition(SlotLocalOffset);
	}

	return GetActorLocation();
}

FVector AFlyingEnemy::ResolveMoveDirection() const
{
	if (MoveMode == EFlyingEnemyMoveMode::TowardTarget && IsValid(TargetActor))
	{
		return (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	}

	return MoveDirection.GetSafeNormal();
}

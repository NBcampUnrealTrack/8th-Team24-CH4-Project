// Copyright Epic Games, Inc. All Rights Reserved.

#include "Obstacles/RunnerObstacle.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/Pawn.h"

ARunnerObstacle::ARunnerObstacle()
{
	PrimaryActorTick.bCanEverTick = false;

	// ── 네트워크: 서버가 스폰하면 클라에 자동 복제 ──
	bReplicates = true;
	bAlwaysRelevant = true;
	// 스폰 후 안 움직이므로 SetReplicateMovement 생략 (스폰 위치는 복제에 포함)


	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(SceneRoot);
	CollisionBox->SetBoxExtent(FVector(80.0f, 80.0f, 120.0f));
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	CollisionBox->SetGenerateOverlapEvents(true);

	CurrentHP = MaxHP;
}

void ARunnerObstacle::BeginPlay()
{
	Super::BeginPlay();

	// 충돌 판정은 서버 전용
	if (!HasAuthority())
	{
		return;
	}

	CurrentHP = MaxHP;
	RefreshCollisionResponses();
	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &ARunnerObstacle::HandleCollisionBeginOverlap);
}

void ARunnerObstacle::InitializeObstacle(ERunnerObstacleActionType InActionType, ERunnerLaneCoverage InLaneCoverage, ERunnerLane InLane)
{
	if (!HasAuthority())
	{
		return;
	}

	ActionType = InActionType;
	LaneCoverage = InLaneCoverage;
	Lane = InLaneCoverage == ERunnerLaneCoverage::AllLanes ? ERunnerLane::All : InLane;
	CurrentHP = MaxHP;
	RefreshCollisionResponses();
}

bool ARunnerObstacle::IsPlayerActor(AActor* Actor) const
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

float ARunnerObstacle::TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (Damage <= 0.0f || !IsShootRequiredObstacle())
	{
		return 0.0f;
	}

	ApplyDamage(Damage, DamageCauser, GetActorLocation(), FVector::ZeroVector);
	return Damage;
}

void ARunnerObstacle::ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse)
{
	if (!HasAuthority())
	{
		return;
	}

	if (Damage <= 0.0f || !IsShootRequiredObstacle() || CurrentHP <= 0.0f)
	{
		return;
	}

	CurrentHP = FMath::Max(0.0f, CurrentHP - Damage);
	OnObstacleDamaged.Broadcast(this, Damage, CurrentHP, DamageCauser);

	if (CurrentHP <= 0.0f)
	{
		HandleDeath();
	}
}

void ARunnerObstacle::HandleDeath()
{
	OnObstacleDestroyed.Broadcast(this);
	Destroy();
}

void ARunnerObstacle::ApplyHealing(float Healing, AActor* Healer)
{
	if (Healing > 0.0f && IsShootRequiredObstacle() && CurrentHP > 0.0f)
	{
		CurrentHP = FMath::Min(MaxHP, CurrentHP + Healing);
	}
}

void ARunnerObstacle::NotifyDanger(const FVector& DangerLocation, AActor* DangerSource)
{
}

void ARunnerObstacle::HandleCollisionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (IsPlayerActor(OtherActor))
	{
		OnPlayerHitObstacle.Broadcast(this, OtherActor, ActionType, LaneCoverage, Lane);
	}
}

void ARunnerObstacle::RefreshCollisionResponses()
{
	if (!CollisionBox)
	{
		return;
	}

	CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionBox->SetCollisionResponseToChannel(ECC_Visibility, IsShootRequiredObstacle() && bBlockVisibilityWhenShootRequired ? ECR_Block : ECR_Ignore);
	CollisionBox->SetGenerateOverlapEvents(true);
}

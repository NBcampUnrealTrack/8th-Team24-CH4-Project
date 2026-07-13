// Copyright Epic Games, Inc. All Rights Reserved.

#include "Obstacles/RunnerObstacle.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
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

	// 장애물 기본 데미지 초기화
	ObstacleDamage = 1.0f;
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
	bDeathHandled = false;

	RefreshCollisionResponses();
	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &ARunnerObstacle::HandleCollisionBeginOverlap);
}

void ARunnerObstacle::Multicast_PlayDestruction_Implementation()
{
	// 기본 메쉬 돌 숨기기
	if (MeshComponent)
	{
		MeshComponent->SetVisibility(false);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 조각내기는 BP에서
	BP_PlayDestruction();
}

void ARunnerObstacle::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	//UE_LOG(LogTemp, Error, TEXT("[사망] EndPlay 실제 파괴됨! Reason=%d"), (int32)EndPlayReason);
	Super::EndPlay(EndPlayReason);
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
	bDeathHandled = false;

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

	if (Damage <= 0.0f || !IsShootRequiredObstacle() || CurrentHP <= 0.0f || bDeathHandled)
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
	if (bDeathHandled)
	{
		return;
	}

	bDeathHandled = true;

	if (bDisableCollisionOnDeath && CollisionBox)
	{
		CollisionBox->SetGenerateOverlapEvents(false);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 서버 + 모든 클라이언트에서 메쉬 숨김 / BP 파괴 연출 실행
	Multicast_PlayDestruction();

	// 서버 + 모든 클라이언트에서 파괴 사운드 즉시 재생
	MulticastPlayDestroySound(GetActorLocation());

	OnObstacleDestroyed.Broadcast(this);

	if (DestroyDelayAfterDeath <= 0.0f)
	{
		Destroy();
		return;
	}

	SetLifeSpan(DestroyDelayAfterDeath);
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
	//로그 확인용
	//UE_LOG(LogTemp, Warning, TEXT("[OBSTACLE] 무언가 닿음: %s"), OtherActor ? *OtherActor->GetName() : TEXT("None"));

	if (IsPlayerActor(OtherActor))
	{
		//UE_LOG(LogTemp, Warning, TEXT("[OBSTACLE] 닿은 액터가 플레이어로 판정됨! 델리게이트 발송."));
		OnPlayerHitObstacle.Broadcast(this, OtherActor, ActionType, LaneCoverage, Lane);
	}
	//else
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("[OBSTACLE] 닿았지만 플레이어가 아님."));
	//}
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

	// [한길] 0713
	// ShootRequired 장애물은 서버에서도 Projectile을 Block해야 투사체 데미지를 받을 수 있습니다.
	CollisionBox->SetCollisionResponseToChannel(ECC_GameTraceChannel1, IsShootRequiredObstacle() ? ECR_Block : ECR_Ignore);

	CollisionBox->SetGenerateOverlapEvents(true);
}

void ARunnerObstacle::MulticastPlayDestroySound_Implementation(FVector SoundLocation)
{
	if (!DestroySound)
	{
		return;
	}

	UGameplayStatics::PlaySoundAtLocation(
		this,
		DestroySound,
		SoundLocation,
		DestroySoundVolume,
		DestroySoundPitch
	);
}

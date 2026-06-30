// Copyright Epic Games, Inc. All Rights Reserved.

#include "Enemies/FlyingEnemySpawner.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Enemies/FlyingEnemy.h"
#include "TimerManager.h"

AFlyingEnemySpawner::AFlyingEnemySpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	SpawnArea = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnArea"));
	SpawnArea->SetupAttachment(SceneRoot);
	SpawnArea->SetBoxExtent(FVector(150.0f, 300.0f, 150.0f));
	SpawnArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AFlyingEnemySpawner::BeginPlay()
{
	Super::BeginPlay();

	RandomStream.Initialize(bUseRandomSeed ? RandomSeed : FMath::Rand());

	if (bAutoStart)
	{
		StartSpawning();
	}
}

void AFlyingEnemySpawner::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	StopSpawning();
	Super::EndPlay(EndPlayReason);
}

void AFlyingEnemySpawner::StartSpawning()
{
	if (!GetWorld())
	{
		return;
	}

	GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AFlyingEnemySpawner::SpawnTimerElapsed, SpawnInterval, true);
}

void AFlyingEnemySpawner::StopSpawning()
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	}
}

AFlyingEnemy* AFlyingEnemySpawner::SpawnOnce()
{
	CleanActiveEnemies();
	if (ActiveEnemies.Num() >= FMath::Clamp(MaxActiveEnemies, 1, 3))
	{
		return nullptr;
	}

	const FFlyingEnemySpawnGroup* Group = ChooseSpawnGroup();
	if (!Group)
	{
		return nullptr;
	}

	const TSubclassOf<AFlyingEnemy> EnemyClass = ChooseEnemyClass(*Group);
	if (!EnemyClass)
	{
		return nullptr;
	}

	const FTransform ReferenceTransform = GetReferenceTransform();
	EFlyingEnemySlot AssignedSlot = EFlyingEnemySlot::Center;
	if (!ChooseAvailableSlot(AssignedSlot))
	{
		return nullptr;
	}

	const FVector SlotLocalOffset = GetSlotLocalOffset(AssignedSlot);
	const FVector SpawnLocation = GetSpawnLocation(ReferenceTransform, AssignedSlot);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;

	AFlyingEnemy* SpawnedEnemy = GetWorld()->SpawnActor<AFlyingEnemy>(EnemyClass, SpawnLocation, ReferenceTransform.Rotator(), SpawnParams);
	if (SpawnedEnemy)
	{
		SpawnedEnemy->ConfigureSlotMovement(SpawnReferenceComponent, SpawnReferenceActor, AssignedSlot, SlotLocalOffset, DescendSpeed, SlotArrivalTolerance, AttackDelayAfterArrive, AttackDamage, GetAttackTarget(), bSpawnedEnemiesFollowReference);
		RegisterSpawnedEnemy(SpawnedEnemy);
		OnFlyingEnemySpawned.Broadcast(this, SpawnedEnemy);
	}

	return SpawnedEnemy;
}

bool AFlyingEnemySpawner::HasValidSpawnGroup() const
{
	for (const FFlyingEnemySpawnGroup& Group : SpawnGroups)
	{
		if (Group.Weight > 0.0f && !Group.EnemyClasses.IsEmpty())
		{
			return true;
		}
	}

	return false;
}

const FFlyingEnemySpawnGroup* AFlyingEnemySpawner::ChooseSpawnGroup()
{
	float TotalWeight = 0.0f;
	for (const FFlyingEnemySpawnGroup& Group : SpawnGroups)
	{
		if (Group.Weight > 0.0f && !Group.EnemyClasses.IsEmpty())
		{
			TotalWeight += Group.Weight;
		}
	}

	if (TotalWeight <= 0.0f)
	{
		return nullptr;
	}

	float Pick = GetRandomRange(0.0f, TotalWeight);
	for (const FFlyingEnemySpawnGroup& Group : SpawnGroups)
	{
		if (Group.Weight <= 0.0f || Group.EnemyClasses.IsEmpty())
		{
			continue;
		}

		Pick -= Group.Weight;
		if (Pick <= 0.0f)
		{
			return &Group;
		}
	}

	return nullptr;
}

TSubclassOf<AFlyingEnemy> AFlyingEnemySpawner::ChooseEnemyClass(const FFlyingEnemySpawnGroup& Group)
{
	if (Group.EnemyClasses.IsEmpty())
	{
		return nullptr;
	}

	return Group.EnemyClasses[GetRandomIndex(Group.EnemyClasses.Num())];
}

bool AFlyingEnemySpawner::ChooseAvailableSlot(EFlyingEnemySlot& OutSlot)
{
	CleanActiveEnemies();

	TArray<EFlyingEnemySlot> AvailableSlots;
	if (!IsSlotOccupied(EFlyingEnemySlot::Left))
	{
		AvailableSlots.Add(EFlyingEnemySlot::Left);
	}
	if (!IsSlotOccupied(EFlyingEnemySlot::Center))
	{
		AvailableSlots.Add(EFlyingEnemySlot::Center);
	}
	if (!IsSlotOccupied(EFlyingEnemySlot::Right))
	{
		AvailableSlots.Add(EFlyingEnemySlot::Right);
	}

	const int32 AllowedSlots = FMath::Clamp(MaxActiveEnemies, 1, 3);
	while (AvailableSlots.Num() > AllowedSlots)
	{
		AvailableSlots.RemoveAt(AvailableSlots.Num() - 1);
	}

	if (AvailableSlots.IsEmpty())
	{
		return false;
	}

	OutSlot = AvailableSlots[GetRandomIndex(AvailableSlots.Num())];
	return true;
}

bool AFlyingEnemySpawner::IsSlotOccupied(EFlyingEnemySlot Slot) const
{
	for (const TWeakObjectPtr<AFlyingEnemy>& Enemy : ActiveEnemies)
	{
		if (Enemy.IsValid() && Enemy->GetAssignedSlot() == Slot)
		{
			return true;
		}
	}

	return false;
}

FTransform AFlyingEnemySpawner::GetReferenceTransform() const
{
	if (IsValid(SpawnReferenceComponent))
	{
		return SpawnReferenceComponent->GetComponentTransform();
	}

	if (IsValid(SpawnReferenceActor))
	{
		return SpawnReferenceActor->GetActorTransform();
	}

	return GetActorTransform();
}

FVector AFlyingEnemySpawner::GetSlotLocalOffset(EFlyingEnemySlot Slot) const
{
	float SideOffset = CenterSlotOffset;
	if (Slot == EFlyingEnemySlot::Left)
	{
		SideOffset = LeftSlotOffset;
	}
	else if (Slot == EFlyingEnemySlot::Right)
	{
		SideOffset = RightSlotOffset;
	}

	return FVector(ForwardDistance, SideOffset, HeightOffset);
}

FVector AFlyingEnemySpawner::GetSpawnLocation(const FTransform& ReferenceTransform, EFlyingEnemySlot Slot) const
{
	const FVector SlotWorldLocation = ReferenceTransform.TransformPosition(GetSlotLocalOffset(Slot));
	return SlotWorldLocation + ReferenceTransform.GetRotation().GetUpVector() * SpawnAboveOffset;
}

AActor* AFlyingEnemySpawner::GetAttackTarget() const
{
	if (IsValid(AttackTargetActor))
	{
		return AttackTargetActor;
	}

	return SpawnReferenceActor.Get();
}

void AFlyingEnemySpawner::RegisterSpawnedEnemy(AFlyingEnemy* Enemy)
{
	if (IsValid(Enemy))
	{
		ActiveEnemies.Add(Enemy);
	}
}

void AFlyingEnemySpawner::CleanActiveEnemies()
{
	ActiveEnemies.RemoveAll([](const TWeakObjectPtr<AFlyingEnemy>& Enemy)
	{
		return !Enemy.IsValid();
	});
}

void AFlyingEnemySpawner::HandleRunnerTurnSucceeded(float DestroyDelay)
{
	CleanActiveEnemies();

	const float ResolvedDestroyDelay = DestroyDelay >= 0.0f ? DestroyDelay : DestroyDelayAfterTurn;
	for (const TWeakObjectPtr<AFlyingEnemy>& Enemy : ActiveEnemies)
	{
		if (Enemy.IsValid())
		{
			Enemy->DisableEnemyInteraction();
			Enemy->StopFollowingReference();
			Enemy->ScheduleDestroy(ResolvedDestroyDelay);
		}
	}
}

void AFlyingEnemySpawner::SpawnTimerElapsed()
{
	SpawnOnce();
}

float AFlyingEnemySpawner::GetRandomRange(float Min, float Max)
{
	return bUseRandomSeed ? RandomStream.FRandRange(Min, Max) : FMath::FRandRange(Min, Max);
}

int32 AFlyingEnemySpawner::GetRandomIndex(int32 Num)
{
	return Num > 0 ? (bUseRandomSeed ? RandomStream.RandRange(0, Num - 1) : FMath::RandRange(0, Num - 1)) : 0;
}

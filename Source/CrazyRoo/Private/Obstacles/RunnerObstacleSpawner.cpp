// Copyright Epic Games, Inc. All Rights Reserved.

#include "Obstacles/RunnerObstacleSpawner.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Obstacles/RunnerObstacle.h"
#include "TimerManager.h"

ARunnerObstacleSpawner::ARunnerObstacleSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	SpawnArea = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnArea"));
	SpawnArea->SetupAttachment(SceneRoot);
	SpawnArea->SetBoxExtent(FVector(150.0f, 450.0f, 100.0f));
	SpawnArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ARunnerObstacleSpawner::BeginPlay()
{
	Super::BeginPlay();

	RandomStream.Initialize(bUseRandomSeed ? RandomSeed : FMath::Rand());

	if (bAutoStart)
	{
		StartSpawning();
	}
}

void ARunnerObstacleSpawner::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	StopSpawning();
	Super::EndPlay(EndPlayReason);
}

void ARunnerObstacleSpawner::StartSpawning()
{
	if (!GetWorld())
	{
		return;
	}

	GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &ARunnerObstacleSpawner::SpawnTimerElapsed, SpawnInterval, true);
}

void ARunnerObstacleSpawner::StopSpawning()
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	}
}

ARunnerObstacle* ARunnerObstacleSpawner::SpawnOnce()
{
	const FRunnerObstacleSpawnGroup* Group = ChooseSpawnGroup();
	if (!Group)
	{
		return nullptr;
	}

	const TSubclassOf<ARunnerObstacle> ObstacleClass = ChooseObstacleClass(*Group);
	if (!ObstacleClass)
	{
		return nullptr;
	}

	const ERunnerLane SpawnLane = ChooseLane(Group->LaneCoverage);
	const FVector SpawnLocation = GetSpawnLocation(SpawnLane);
	const FRotator SpawnRotation = GetActorRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;

	ARunnerObstacle* SpawnedObstacle = GetWorld()->SpawnActor<ARunnerObstacle>(ObstacleClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (SpawnedObstacle)
	{
		SpawnedObstacle->InitializeObstacle(Group->ActionType, Group->LaneCoverage, SpawnLane);
		OnObstacleSpawned.Broadcast(this, SpawnedObstacle);
	}

	return SpawnedObstacle;
}

bool ARunnerObstacleSpawner::HasValidSpawnGroup() const
{
	for (const FRunnerObstacleSpawnGroup& Group : SpawnGroups)
	{
		if (Group.Weight > 0.0f && !Group.ObstacleClasses.IsEmpty())
		{
			return true;
		}
	}

	return false;
}

const FRunnerObstacleSpawnGroup* ARunnerObstacleSpawner::ChooseSpawnGroup()
{
	float TotalWeight = 0.0f;
	for (const FRunnerObstacleSpawnGroup& Group : SpawnGroups)
	{
		if (Group.Weight > 0.0f && !Group.ObstacleClasses.IsEmpty())
		{
			TotalWeight += Group.Weight;
		}
	}

	if (TotalWeight <= 0.0f)
	{
		return nullptr;
	}

	float Pick = GetRandomRange(0.0f, TotalWeight);
	for (const FRunnerObstacleSpawnGroup& Group : SpawnGroups)
	{
		if (Group.Weight <= 0.0f || Group.ObstacleClasses.IsEmpty())
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

TSubclassOf<ARunnerObstacle> ARunnerObstacleSpawner::ChooseObstacleClass(const FRunnerObstacleSpawnGroup& Group)
{
	if (Group.ObstacleClasses.IsEmpty())
	{
		return nullptr;
	}

	return Group.ObstacleClasses[GetRandomIndex(Group.ObstacleClasses.Num())];
}

ERunnerLane ARunnerObstacleSpawner::ChooseLane(ERunnerLaneCoverage LaneCoverage)
{
	if (LaneCoverage == ERunnerLaneCoverage::AllLanes)
	{
		return ERunnerLane::All;
	}

	switch (GetRandomIndex(3))
	{
	case 0:
		return ERunnerLane::Left;
	case 1:
		return ERunnerLane::Center;
	default:
		return ERunnerLane::Right;
	}
}

FVector ARunnerObstacleSpawner::GetSpawnLocation(ERunnerLane Lane)
{
	float LaneOffset = 0.0f;
	if (Lane == ERunnerLane::Left)
	{
		LaneOffset = -LaneSpacing;
	}
	else if (Lane == ERunnerLane::Right)
	{
		LaneOffset = LaneSpacing;
	}

	const FVector Extent = SpawnArea->GetScaledBoxExtent();
	const FVector LocalOffset(GetRandomRange(-Extent.X, Extent.X), LaneOffset, GetRandomRange(-Extent.Z, Extent.Z));
	return SpawnArea->GetComponentTransform().TransformPosition(LocalOffset);
}

void ARunnerObstacleSpawner::SpawnTimerElapsed()
{
	SpawnOnce();
}

float ARunnerObstacleSpawner::GetRandomRange(float Min, float Max)
{
	return bUseRandomSeed ? RandomStream.FRandRange(Min, Max) : FMath::FRandRange(Min, Max);
}

int32 ARunnerObstacleSpawner::GetRandomIndex(int32 Num)
{
	return Num > 0 ? (bUseRandomSeed ? RandomStream.RandRange(0, Num - 1) : FMath::RandRange(0, Num - 1)) : 0;
}

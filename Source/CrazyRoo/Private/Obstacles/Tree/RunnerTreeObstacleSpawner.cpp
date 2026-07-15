// Copyright Epic Games, Inc. All Rights Reserved.

#include "Obstacles/Tree/RunnerTreeObstacleSpawner.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Obstacles/Tree/RunnerFallingTreeObstacle.h"
#include "Obstacles/Tree/RunnerTreeObstacleTypes.h"
#include "Obstacles/Tree/RunnerTreeSpawnPoint.h"

namespace
{
template <typename ElementType>
void RunnerShuffleArray(TArray<ElementType>& Array)
{
	for (int32 Index = Array.Num() - 1; Index > 0; --Index)
	{
		const int32 SwapIndex = FMath::RandRange(0, Index);
		Array.Swap(Index, SwapIndex);
	}
}
}

ARunnerTreeObstacleSpawner::ARunnerTreeObstacleSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARunnerTreeObstacleSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	if (bSpawnOnBeginPlay)
	{
		SpawnTrees();
	}
}

void ARunnerTreeObstacleSpawner::SpawnTrees()
{
	if (!HasAuthority())
	{
		return;
	}

	if (!TreeObstacleClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunnerTreeObstacleSpawner] TreeObstacleClass is null"));
		return;
	}

	TArray<ARunnerTreeSpawnPoint*> EarlyPoints;
	TArray<ARunnerTreeSpawnPoint*> LatePoints;
	CollectSpawnPoints(EarlyPoints, LatePoints);

	UE_LOG(LogTemp, Log, TEXT("[RunnerTreeObstacleSpawner] Collected Early=%d Late=%d"), EarlyPoints.Num(), LatePoints.Num());

	RunnerShuffleArray(EarlyPoints);
	RunnerShuffleArray(LatePoints);

	SpawnTreesFromPoints(EarlyPoints, EarlySpawnCount);
	SpawnTreesFromPoints(LatePoints, LateSpawnCount);
}

void ARunnerTreeObstacleSpawner::CollectSpawnPoints(TArray<ARunnerTreeSpawnPoint*>& OutEarlyPoints, TArray<ARunnerTreeSpawnPoint*>& OutLatePoints) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<ARunnerTreeSpawnPoint> It(World); It; ++It)
	{
		ARunnerTreeSpawnPoint* SpawnPoint = *It;
		if (!IsValid(SpawnPoint))
		{
			continue;
		}

		if (SpawnPoint->GetSpawnSection() == ERunnerTreeSpawnSection::Early)
		{
			OutEarlyPoints.Add(SpawnPoint);
		}
		else
		{
			OutLatePoints.Add(SpawnPoint);
		}
	}
}

void ARunnerTreeObstacleSpawner::SpawnTreesFromPoints(TArray<ARunnerTreeSpawnPoint*>& SpawnPoints, int32 SpawnCount)
{
	UWorld* World = GetWorld();
	if (!World || !TreeObstacleClass)
	{
		return;
	}

	const int32 SafeSpawnCount = FMath::Min(FMath::Max(0, SpawnCount), SpawnPoints.Num());
	for (int32 Index = 0; Index < SafeSpawnCount; ++Index)
	{
		ARunnerTreeSpawnPoint* SpawnPoint = SpawnPoints[Index];
		if (!IsValid(SpawnPoint))
		{
			continue;
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ARunnerFallingTreeObstacle* SpawnedTree = World->SpawnActor<ARunnerFallingTreeObstacle>(
			TreeObstacleClass,
			SpawnPoint->GetSpawnTransform(),
			SpawnParameters
		);

		if (SpawnedTree)
		{
			UE_LOG(LogTemp, Log, TEXT("[RunnerTreeObstacleSpawner] Spawned falling tree: %s"), *SpawnedTree->GetName());
		}
	}
}

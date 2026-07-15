// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RunnerTreeObstacleSpawner.generated.h"

class ARunnerFallingTreeObstacle;
class ARunnerTreeSpawnPoint;

UCLASS(Blueprintable)
class CRAZYROO_API ARunnerTreeObstacleSpawner : public AActor
{
	GENERATED_BODY()

public:
	ARunnerTreeObstacleSpawner();

	UFUNCTION(BlueprintCallable, Category = "Tree Spawn")
	void SpawnTrees();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree Spawn")
	TSubclassOf<ARunnerFallingTreeObstacle> TreeObstacleClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree Spawn")
	int32 EarlySpawnCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree Spawn")
	int32 LateSpawnCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree Spawn")
	bool bSpawnOnBeginPlay = true;

	void CollectSpawnPoints(TArray<ARunnerTreeSpawnPoint*>& OutEarlyPoints, TArray<ARunnerTreeSpawnPoint*>& OutLatePoints) const;
	void SpawnTreesFromPoints(TArray<ARunnerTreeSpawnPoint*>& SpawnPoints, int32 SpawnCount);
};

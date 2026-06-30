// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RunnerTypes.h"
#include "RunnerObstacleSpawner.generated.h"

class UBoxComponent;
class USceneComponent;
class ARunnerObstacleSpawner;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRunnerObstacleSpawnedSignature, ARunnerObstacleSpawner*, Spawner, ARunnerObstacle*, SpawnedObstacle);

UCLASS(Blueprintable)
class CRAZYROO_API ARunnerObstacleSpawner : public AActor
{
	GENERATED_BODY()

public:
	ARunnerObstacleSpawner();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* SpawnArea;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Spawn")
	TArray<FRunnerObstacleSpawnGroup> SpawnGroups;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Spawn", meta = (ClampMin = "0.0", Units = "cm"))
	float LaneSpacing = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Spawn", meta = (ClampMin = "0.01", Units = "s"))
	float SpawnInterval = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Spawn")
	bool bAutoStart = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Spawn")
	bool bUseRandomSeed = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Spawn", meta = (EditCondition = "bUseRandomSeed"))
	int32 RandomSeed = 12345;

	FRandomStream RandomStream;
	FTimerHandle SpawnTimerHandle;

public:
	UPROPERTY(BlueprintAssignable, Category = "Obstacle Spawn|Events")
	FRunnerObstacleSpawnedSignature OnObstacleSpawned;

	UFUNCTION(BlueprintCallable, Category = "Obstacle Spawn")
	void StartSpawning();

	UFUNCTION(BlueprintCallable, Category = "Obstacle Spawn")
	void StopSpawning();

	UFUNCTION(BlueprintCallable, Category = "Obstacle Spawn")
	ARunnerObstacle* SpawnOnce();

	UFUNCTION(BlueprintPure, Category = "Obstacle Spawn")
	bool HasValidSpawnGroup() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	const FRunnerObstacleSpawnGroup* ChooseSpawnGroup();
	TSubclassOf<ARunnerObstacle> ChooseObstacleClass(const FRunnerObstacleSpawnGroup& Group);
	ERunnerLane ChooseLane(ERunnerLaneCoverage LaneCoverage);
	FVector GetSpawnLocation(ERunnerLane Lane);
	void SpawnTimerElapsed();
	float GetRandomRange(float Min, float Max);
	int32 GetRandomIndex(int32 Num);
};

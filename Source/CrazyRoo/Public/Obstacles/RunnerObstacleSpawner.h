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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Spawn", meta = (ClampMin = "0", ClampMax = "3"))
	int32 MaxAllLaneObstacleCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Spawn", meta = (ClampMin = "0", ClampMax = "2"))
	int32 MaxSingleLaneObstacleCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Spawn")
	bool bGenerateOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Spawn")
	bool bAllowRegenerate = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Obstacle Spawn")
	bool bHasGenerated = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Spawn")
	bool bUseRandomSeed = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Spawn", meta = (EditCondition = "bUseRandomSeed"))
	int32 RandomSeed = 12345;

	FRandomStream RandomStream;
	TArray<TWeakObjectPtr<ARunnerObstacle>> GeneratedObstacles;

public:
	UPROPERTY(BlueprintAssignable, Category = "Obstacle Spawn|Events")
	FRunnerObstacleSpawnedSignature OnObstacleSpawned;

	UFUNCTION(BlueprintCallable, Category = "Obstacle Spawn")
	void StartSpawning();

	UFUNCTION(BlueprintCallable, Category = "Obstacle Spawn")
	void StopSpawning();

	UFUNCTION(BlueprintCallable, Category = "Obstacle Spawn")
	ARunnerObstacle* SpawnOnce();

	UFUNCTION(BlueprintCallable, Category = "Obstacle Spawn")
	int32 GenerateObstaclesOnce();

	UFUNCTION(BlueprintCallable, Category = "Obstacle Spawn")
	void ResetGeneratedObstacles(bool bDestroyExisting = false);

	UFUNCTION(BlueprintPure, Category = "Obstacle Spawn")
	bool HasValidSpawnGroup() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	const FRunnerObstacleSpawnGroup* ChooseSpawnGroup();
	const FRunnerObstacleSpawnGroup* ChooseSpawnGroupByCoverage(ERunnerLaneCoverage DesiredCoverage);
	bool ChooseGenerationCoverage(ERunnerLaneCoverage& OutCoverage);
	float GetTotalWeightByCoverage(ERunnerLaneCoverage DesiredCoverage) const;
	TSubclassOf<ARunnerObstacle> ChooseObstacleClass(const FRunnerObstacleSpawnGroup& Group);
	ERunnerLane ChooseLane(ERunnerLaneCoverage LaneCoverage);
	bool ChooseUnusedSingleLane(TArray<ERunnerLane>& AvailableLanes, ERunnerLane& OutLane);
	FVector GetSpawnLocation(ERunnerLane Lane);
	ARunnerObstacle* SpawnObstacleFromGroup(const FRunnerObstacleSpawnGroup& Group, ERunnerLane SpawnLane);
	float GetRandomRange(float Min, float Max);
	int32 GetRandomIndex(int32 Num);
};

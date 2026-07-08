// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RunnerTypes.h"
#include "FlyingEnemySpawner.generated.h"

class UBoxComponent;
class USceneComponent;
class AFlyingEnemySpawner;
class AFlyingEnemy;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFlyingEnemySpawnedSignature, AFlyingEnemySpawner*, Spawner, AFlyingEnemy*, SpawnedEnemy);

UCLASS(Blueprintable)
class CRAZYROO_API AFlyingEnemySpawner : public AActor
{
	GENERATED_BODY()

public:
	AFlyingEnemySpawner();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* SpawnArea;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Spawn")
	TArray<FFlyingEnemySpawnGroup> SpawnGroups;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Spawn")
	TObjectPtr<AActor> SpawnReferenceActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Spawn")
	TObjectPtr<USceneComponent> SpawnReferenceComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Spawn")
	TObjectPtr<AActor> AttackTargetActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Spawn")
	bool bSpawnedEnemiesFollowReference = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Spawn|Deprecated", meta = (AdvancedDisplay))
	bool bUseSpawnerAsSlotReference = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Spawn", meta = (ClampMin = "1", ClampMax = "3"))
	int32 MaxActiveEnemies = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Spawn|Slots", meta = (Units = "cm"))
	float LeftSlotOffset = -700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Spawn|Slots", meta = (Units = "cm"))
	float CenterSlotOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Spawn|Slots", meta = (Units = "cm"))
	float RightSlotOffset = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Spawn|Slots", meta = (ClampMin = "0.0", Units = "cm"))
	float ForwardDistance = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Spawn|Slots", meta = (Units = "cm"))
	float HeightOffset = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Spawn|Slots", meta = (ClampMin = "0.0", Units = "cm"))
	float SpawnAboveOffset = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Spawn|Slots", meta = (ClampMin = "0.0", Units = "cm/s"))
	float DescendSpeed = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Spawn|Slots", meta = (ClampMin = "0.0", Units = "cm"))
	float SlotArrivalTolerance = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Spawn|Attack", meta = (ClampMin = "0.0", Units = "s"))
	float AttackDelayAfterArrive = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Spawn|Attack", meta = (ClampMin = "0.0"))
	float AttackDamage = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Spawn|Cleanup", meta = (ClampMin = "0.0", Units = "s"))
	float DestroyDelayAfterTurn = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Spawn", meta = (ClampMin = "0.01", Units = "s"))
	float SpawnInterval = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Spawn")
	bool bAutoStart = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Spawn")
	bool bUseRandomSeed = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Spawn", meta = (EditCondition = "bUseRandomSeed"))
	int32 RandomSeed = 24680;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Spawn|Debug")
	bool bDebugEnemyTracking = false;

	FRandomStream RandomStream;
	FTimerHandle SpawnTimerHandle;
	TArray<TWeakObjectPtr<AFlyingEnemy>> ActiveEnemies;

public:
	UPROPERTY(BlueprintAssignable, Category = "Enemy Spawn|Events")
	FFlyingEnemySpawnedSignature OnFlyingEnemySpawned;

	UFUNCTION(BlueprintCallable, Category = "Enemy Spawn")
	void StartSpawning();

	UFUNCTION(BlueprintCallable, Category = "Enemy Spawn")
	void StopSpawning();

	UFUNCTION(BlueprintCallable, Category = "Enemy Spawn")
	AFlyingEnemy* SpawnOnce();

	UFUNCTION(BlueprintCallable, Category = "Enemy Spawn")
	void HandleRunnerTurnSucceeded(float DestroyDelay = -1.0f);

	UFUNCTION(BlueprintPure, Category = "Enemy Spawn")
	bool HasValidSpawnGroup() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	const FFlyingEnemySpawnGroup* ChooseSpawnGroup();
	TSubclassOf<AFlyingEnemy> ChooseEnemyClass(const FFlyingEnemySpawnGroup& Group);
	bool ChooseAvailableSlot(EFlyingEnemySlot& OutSlot);
	bool IsSlotOccupied(EFlyingEnemySlot Slot) const;
	FTransform GetReferenceTransform() const;
	FTransform GetSlotReferenceTransform() const;
	bool ResolveSpawnReference();
	FVector GetSlotLocalOffset(EFlyingEnemySlot Slot) const;
	FVector GetSpawnLocation(const FTransform& ReferenceTransform, EFlyingEnemySlot Slot) const;
	FRotator GetLookAtReferenceRotation(const FVector& SpawnLocation, const FTransform& ReferenceTransform) const;
	AActor* GetSlotReferenceActor() const;
	USceneComponent* GetSlotReferenceComponent() const;
	AActor* GetAttackTarget() const;
	void RegisterSpawnedEnemy(AFlyingEnemy* Enemy);
	void CleanActiveEnemies();
	void SpawnTimerElapsed();
	float GetRandomRange(float Min, float Max);
	int32 GetRandomIndex(int32 Num);
};

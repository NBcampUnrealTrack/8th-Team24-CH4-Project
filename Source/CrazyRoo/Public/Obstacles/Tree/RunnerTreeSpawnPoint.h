// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Obstacles/Tree/RunnerTreeObstacleTypes.h"
#include "RunnerTreeSpawnPoint.generated.h"

class UArrowComponent;
class UBoxComponent;
class USceneComponent;

UCLASS(Blueprintable)
class CRAZYROO_API ARunnerTreeSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	ARunnerTreeSpawnPoint();

	UFUNCTION(BlueprintCallable, Category = "Tree Spawn")
	ERunnerTreeSpawnSection GetSpawnSection() const;

	UFUNCTION(BlueprintCallable, Category = "Tree Spawn")
	FTransform GetSpawnTransform() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SpawnTransform;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UArrowComponent> ArrowComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> EditorBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree Spawn")
	ERunnerTreeSpawnSection SpawnSection = ERunnerTreeSpawnSection::Early;
};

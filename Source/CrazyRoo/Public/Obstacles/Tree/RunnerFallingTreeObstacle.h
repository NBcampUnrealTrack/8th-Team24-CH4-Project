// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Obstacles/Tree/RunnerTreeObstacleTypes.h"
#include "TimerManager.h"
#include "RunnerFallingTreeObstacle.generated.h"

class ASharedRailCharacter;
class UBoxComponent;
class UPrimitiveComponent;
class USceneComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class CRAZYROO_API ARunnerFallingTreeObstacle : public AActor
{
	GENERATED_BODY()

public:
	ARunnerFallingTreeObstacle();

	UFUNCTION(BlueprintCallable, Category = "Tree")
	void StartFalling();

	UFUNCTION()
	void OnRep_FallState();

	UFUNCTION(BlueprintImplementableEvent, Category = "Tree|Animation")
	void BP_OnTreeFallStarted(float ServerStartTime);

	UFUNCTION(BlueprintImplementableEvent, Category = "Tree|Animation")
	void BP_OnTreeImpacted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Tree|Animation")
	void BP_OnTreeResetToStanding();

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> TreePivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> TreeMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> TriggerBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> DamageBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> FallenBlockingBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree|Damage")
	float TreeDamage = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree|Damage")
	bool bIgnoreInvincibility = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree|Timing", meta = (ClampMin = "0.0", Units = "s"))
	float ImpactTime = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree|Timing", meta = (ClampMin = "0.0", Units = "s"))
	float DamageActiveDuration = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree|Debug")
	bool bDebugTreeObstacle = false;

	UPROPERTY(ReplicatedUsing = OnRep_FallState)
	FRunnerTreeFallRepData FallState;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> DamagedActors;

	UFUNCTION()
	void HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleDamageBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleFallenBlockingBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void HandleImpact();
	void DisableDamageBox();
	void ApplyTreeHit(AActor* Actor);
	bool IsValidRunnerPlayer(AActor* Actor) const;
	float GetServerTime() const;

private:
	FTimerHandle ImpactTimerHandle;
	FTimerHandle DisableDamageBoxTimerHandle;
};

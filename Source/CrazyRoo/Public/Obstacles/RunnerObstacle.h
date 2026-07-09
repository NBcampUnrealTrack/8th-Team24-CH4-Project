// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CombatDamageable.h"
#include "RunnerTypes.h"
#include "RunnerObstacle.generated.h"

class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;
class ARunnerObstacle;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FRunnerObstaclePlayerOverlapSignature, ARunnerObstacle*, Obstacle, AActor*, PlayerActor, ERunnerObstacleActionType, ActionType, ERunnerLaneCoverage, LaneCoverage, ERunnerLane, Lane);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FRunnerObstacleDamageSignature, ARunnerObstacle*, Obstacle, float, Damage, float, CurrentHP, AActor*, DamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRunnerObstacleDestroyedSignature, ARunnerObstacle*, Obstacle);

UCLASS(Blueprintable)
class CRAZYROO_API ARunnerObstacle : public AActor, public ICombatDamageable
{
	GENERATED_BODY()

public:
	ARunnerObstacle();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* CollisionBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle")
	ERunnerObstacleActionType ActionType = ERunnerObstacleActionType::LaneDodge;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle")
	ERunnerLaneCoverage LaneCoverage = ERunnerLaneCoverage::SingleLane;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle")
	ERunnerLane Lane = ERunnerLane::Center;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle|Damage", meta = (ClampMin = "0.0"))
	float MaxHP = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Obstacle|Damage")
	float CurrentHP = 1.0f;

	// 장애물이 플레이어와 충돌했을 때 입힐 데미지 변수 (에디터에서 수정 가능)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle|Damage", meta = (ClampMin = "0.0"))
	float ObstacleDamage = 1.0f;

	// HP가 0이 된 뒤 실제 Destroy까지 기다릴 시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle|Death", meta = (ClampMin = "0.0", Units = "s"))
	float DestroyDelayAfterDeath = 1.0f;

	// 파괴 처리 시작 시 충돌 판정 on/off
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle|Death")
	bool bDisableCollisionOnDeath = true;

	// 파괴 처리 중복 방지
	bool bDeathHandled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle|Collision")
	bool bBlockVisibilityWhenShootRequired = true;

public:
	UPROPERTY(BlueprintAssignable, Category = "Obstacle|Events")
	FRunnerObstaclePlayerOverlapSignature OnPlayerHitObstacle;

	UPROPERTY(BlueprintAssignable, Category = "Obstacle|Events")
	FRunnerObstacleDamageSignature OnObstacleDamaged;

	UPROPERTY(BlueprintAssignable, Category = "Obstacle|Events")
	FRunnerObstacleDestroyedSignature OnObstacleDestroyed;

	UFUNCTION(BlueprintCallable, Category = "Obstacle")
	void InitializeObstacle(ERunnerObstacleActionType InActionType, ERunnerLaneCoverage InLaneCoverage, ERunnerLane InLane);

	UFUNCTION(BlueprintPure, Category = "Obstacle")
	ERunnerObstacleActionType GetActionType() const { return ActionType; }

	UFUNCTION(BlueprintPure, Category = "Obstacle")
	ERunnerLaneCoverage GetLaneCoverage() const { return LaneCoverage; }

	UFUNCTION(BlueprintPure, Category = "Obstacle")
	ERunnerLane GetLane() const { return Lane; }

	UFUNCTION(BlueprintPure, Category = "Obstacle|Damage")
	float GetCurrentHP() const { return CurrentHP; }

	// [추가] 매니저가 데미지 값을 꺼내갈 수 있도록 하는 Getter 함수
	UFUNCTION(BlueprintPure, Category = "Obstacle|Damage")
	float GetObstacleDamage() const { return ObstacleDamage; }

	UFUNCTION(BlueprintPure, Category = "Obstacle|Damage")
	bool IsShootRequiredObstacle() const { return ActionType == ERunnerObstacleActionType::ShootRequired; }

	UFUNCTION(BlueprintPure, Category = "Obstacle")
	bool IsPlayerActor(AActor* Actor) const;

	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	virtual void ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse) override;
	virtual void HandleDeath() override;
	virtual void ApplyHealing(float Healing, AActor* Healer) override;
	virtual void NotifyDanger(const FVector& DangerLocation, AActor* DangerSource) override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleCollisionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void RefreshCollisionResponses();
};

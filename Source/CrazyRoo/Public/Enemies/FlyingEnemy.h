// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CombatDamageable.h"
#include "RunnerTypes.h"
#include "FlyingEnemy.generated.h"

class UBoxComponent;
class USceneComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class AFlyingEnemy;
class UNiagaraSystem;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FFlyingEnemyDamageSignature, AFlyingEnemy*, Enemy, float, Damage, float, CurrentHP, AActor*, DamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFlyingEnemyDestroyedSignature, AFlyingEnemy*, Enemy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFlyingEnemyPlayerHitSignature, AFlyingEnemy*, Enemy, AActor*, PlayerActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FFlyingEnemyAttackHitPlayerSignature, AFlyingEnemy*, Enemy, AActor*, TargetPlayer, float, Damage);

UCLASS(Blueprintable)
class CRAZYROO_API AFlyingEnemy : public AActor, public ICombatDamageable
{
	GENERATED_BODY()

public:
	AFlyingEnemy();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* StaticMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* SkeletalMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* HitBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Damage", meta = (ClampMin = "0.0"))
	float MaxHP = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Damage")
	float CurrentHP = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement", meta = (ClampMin = "0.0", Units = "cm/s"))
	float MoveSpeed = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement")
	EFlyingEnemyMoveMode MoveMode = EFlyingEnemyMoveMode::FixedDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement")
	FVector MoveDirection = FVector(-1.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement")
	TObjectPtr<AActor> TargetActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement")
	bool bMovementEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Slot")
	bool bFollowReferenceActor = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Slot")
	bool bHasArrivedAtSlot = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Slot")
	EFlyingEnemySlot AssignedSlot = EFlyingEnemySlot::Center;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Slot", meta = (ClampMin = "0.0", Units = "cm/s"))
	float DescendSpeed = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Slot", meta = (ClampMin = "0.0", Units = "cm"))
	float SlotArrivalTolerance = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Attack")
	bool bCanAttack = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Attack", meta = (ClampMin = "0.0", Units = "s"))
	float AttackDelayAfterArrive = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Attack", meta = (ClampMin = "0.0"))
	float AttackDamage = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Attack")
	TObjectPtr<AActor> AttackTargetActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Collision")
	bool bBlockVisibilityChannel = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Lifetime")
	bool bAutoDestroyAfterLifetime = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Lifetime", meta = (ClampMin = "0.0", Units = "s", EditCondition = "bAutoDestroyAfterLifetime"))
	float Lifetime = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Debug")
	bool bDebugEnemyTracking = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Effects")
	TObjectPtr<UNiagaraSystem> HitEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Effects")
	FVector HitEffectScale = FVector(1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Effects")
	TObjectPtr<USoundBase> HitSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Effects", meta = (ClampMin = "0.0"))
	float HitSoundVolume = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Effects", meta = (ClampMin = "0.0"))
	float HitSoundPitch = 1.0f;

	FTimerHandle LifetimeTimerHandle;
	FTimerHandle AttackTimerHandle;
	FTimerHandle DeferredDestroyTimerHandle;
	TWeakObjectPtr<AActor> SlotReferenceActor;
	TWeakObjectPtr<USceneComponent> SlotReferenceComponent;
	FVector SlotLocalOffset = FVector::ZeroVector;
	FVector InitialReferenceLocation = FVector::ZeroVector;
	FVector InitialReferenceForward = FVector::ForwardVector;
	FVector InitialSlotWorldLocation = FVector::ZeroVector;
	bool bUseSlotMovement = false;
	bool bDestroyEventBroadcasted = false;
	bool bAttackTimerStarted = false;
	bool bHasInitialSlotTracking = false;
	float DebugEnemyTrackingLogElapsed = 0.0f;

public:
	UPROPERTY(BlueprintAssignable, Category = "Enemy|Events")
	FFlyingEnemyDamageSignature OnFlyingEnemyDamaged;

	UPROPERTY(BlueprintAssignable, Category = "Enemy|Events")
	FFlyingEnemyDestroyedSignature OnFlyingEnemyDestroyed;

	UPROPERTY(BlueprintAssignable, Category = "Enemy|Events")
	FFlyingEnemyPlayerHitSignature OnFlyingEnemyHitPlayer;

	UPROPERTY(BlueprintAssignable, Category = "Enemy|Events")
	FFlyingEnemyAttackHitPlayerSignature OnFlyingEnemyAttackHitPlayer;

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void InitializeFlyingEnemy(const FVector& InMoveDirection, AActor* InTargetActor);

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void ConfigureSlotMovement(USceneComponent* InReferenceComponent, AActor* InReferenceActor, EFlyingEnemySlot InAssignedSlot, const FVector& InSlotLocalOffset, float InDescendSpeed, float InArrivalTolerance, float InAttackDelayAfterArrive, float InAttackDamage, AActor* InAttackTargetActor, bool bInFollowReferenceActor);

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void SetMovementTarget(AActor* InTargetActor);

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void SetMoveDirection(const FVector& InMoveDirection);

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void SetMoveMode(EFlyingEnemyMoveMode InMoveMode);

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void DisableEnemyInteraction();

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void StopFollowingReference();

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void ScheduleDestroy(float Delay);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Debug")
	void SetDebugEnemyTracking(bool bInDebugEnemyTracking);

	UFUNCTION(BlueprintPure, Category = "Enemy|Slot")
	EFlyingEnemySlot GetAssignedSlot() const { return AssignedSlot; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Damage")
	float GetCurrentHP() const { return CurrentHP; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsPlayerActor(AActor* Actor) const;

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayHitEffect(FVector EffectLocation);

	virtual void Tick(float DeltaSeconds) override;
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	virtual void ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse) override;
	virtual void HandleDeath() override;
	virtual void ApplyHealing(float Healing, AActor* Healer) override;
	virtual void NotifyDanger(const FVector& DangerLocation, AActor* DangerSource) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleHitBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void DestroyByLifetime();
	void TriggerAttackHitPlayer();
	void BroadcastDestroyedEvent();
	void RefreshCollisionResponses();
	void StartAttackDelay();
	FVector GetCurrentSlotWorldLocation() const;
	FVector GetReferenceLocation() const;
	FVector GetLookAtLocation() const;
	bool GetReferenceTransform(FTransform& OutTransform) const;
	void UpdateLookAtReference();
	FVector ResolveMoveDirection() const;
};

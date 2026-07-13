// RailProjectile.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RailProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UNiagaraComponent;
class UProjectileMovementComponent;

UCLASS()
class CRAZYROO_API ARailProjectile : public AActor
{
	GENERATED_BODY()

public:
	ARailProjectile();

	/**
	 * 투사체가 날아갈 방향과 발사 정보를 설정합니다.
	 * 서버가 Projectile을 스폰한 직후 호출합니다.
	 */
	void InitializeProjectile(
		const FVector& ShootDirection,
		float InDamage,
		AController* InInstigatorController
	);

protected:
	virtual void BeginPlay() override;

	// 실제 충돌 판정을 담당하는 루트 컴포넌트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Collision")
	TObjectPtr<USphereComponent> CollisionComponent;

	// 눈에 보이는 총알 Mesh입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Visual")
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

	// 총알 궤적용 Niagara 컴포넌트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Visual")
	TObjectPtr<UNiagaraComponent> TrailEffect;

	// 투사체 이동을 담당합니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Movement")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	// 투사체가 적에게 적용할 데미지입니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Damage")
	float Damage = 20.f;

	// 데미지를 발생시킨 플레이어의 Controller입니다.
	UPROPERTY()
	TObjectPtr<AController> InstigatorController;

	// 충돌했을 때 호출됩니다.
	UFUNCTION()
	void HandleProjectileHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse,
		const FHitResult& Hit
	);
};

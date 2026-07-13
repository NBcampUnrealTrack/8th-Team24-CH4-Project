// RailProjectile.cpp

#include "Weapons/RailProjectile.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"

ARailProjectile::ARailProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	// 서버에서 스폰된 Projectile이 클라이언트에도 생성되도록 합니다.
	bReplicates = true;

	// Projectile 위치와 회전을 복제합니다.
	SetReplicateMovement(true);

	// 충돌 컴포넌트 생성
	CollisionComponent =
		CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));

	RootComponent = CollisionComponent;

	CollisionComponent->InitSphereRadius(8.f);
	CollisionComponent->SetCollisionEnabled(
		ECollisionEnabled::QueryAndPhysics
	);

	/*
	 *Porject Settings에서 별도의 Projectile 채널을 만들었습니다.
	 */
	CollisionComponent->SetCollisionProfileName(TEXT("Projectile"));

	// OnComponentHit 이벤트가 발생하도록 설정합니다.
	CollisionComponent->SetNotifyRigidBodyCollision(true);

	// 시각용 Mesh 생성
	ProjectileMesh =
		CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));

	ProjectileMesh->SetupAttachment(CollisionComponent);

	// 실제 충돌은 Sphere가 처리하므로 Mesh 충돌은 끕니다.
	//ProjectileMesh->SetCollisionEnabled(
	//	ECollisionEnabled::NoCollision
	//);

	// Niagara 궤적 생성
	TrailEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailEffect"));

	TrailEffect->SetupAttachment(CollisionComponent);

	// 이동 컴포넌트 생성
	ProjectileMovement =
		CreateDefaultSubobject<UProjectileMovementComponent>(
			TEXT("ProjectileMovement")
		);

	ProjectileMovement->UpdatedComponent = CollisionComponent;

	// 투사체 속도
	ProjectileMovement->InitialSpeed = 3000.f;
	ProjectileMovement->MaxSpeed = 3000.f;

	// 총알이 중력에 의해 떨어지지 않게 설정
	ProjectileMovement->ProjectileGravityScale = 0.f;

	// 이동 방향을 바라보도록 회전
	ProjectileMovement->bRotationFollowsVelocity = true;

	// 고속 이동 중 충돌 누락 방지에 도움을 줍니다.
	ProjectileMovement->bForceSubStepping = true;

	// 일정 시간 뒤 자동 삭제
	InitialLifeSpan = 1.f;
}

void ARailProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (CollisionComponent)
	{
		CollisionComponent->OnComponentHit.AddDynamic(
			this,
			&ARailProjectile::HandleProjectileHit
		);

		// 발사한 캐릭터와 즉시 충돌하지 않게 합니다.
		if (AActor* OwnerActor = GetOwner())
		{
			CollisionComponent->IgnoreActorWhenMoving(
				OwnerActor,
				true
			);
		}
	}
}

void ARailProjectile::InitializeProjectile(
	const FVector& ShootDirection,
	float InDamage,
	AController* InInstigatorController
)
{
	Damage = InDamage;
	InstigatorController = InInstigatorController;

	if (!ProjectileMovement)
	{
		return;
	}

	const FVector NormalizedDirection =
		ShootDirection.GetSafeNormal();

	// 투사체의 실제 이동 속도를 설정합니다.
	ProjectileMovement->Velocity =
		NormalizedDirection * ProjectileMovement->InitialSpeed;
}

void ARailProjectile::HandleProjectileHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	FVector NormalImpulse,
	const FHitResult& Hit
)
{
	UE_LOG(
	LogTemp,
	Warning,
	TEXT("Projectile Hit!! HasAuthority=%s / OtherActor=%s / OtherComponent=%s / HitLocation=%s / Damage=%.1f"),
	HasAuthority() ? TEXT("true") : TEXT("false"),
	*GetNameSafe(OtherActor),
	*GetNameSafe(OtherComponent),
	*Hit.ImpactPoint.ToString(),
	Damage
);

	// 데미지 판정과 제거는 서버에서만 처리합니다.
	if (!HasAuthority())
	{
		return;
	}

	AActor* OwnerActor = GetOwner();

	if (
		IsValid(OtherActor) &&
		OtherActor != this &&
		OtherActor != OwnerActor
	)
	{
		UGameplayStatics::ApplyDamage(
			OtherActor,
			Damage,
			InstigatorController,
			this,
			nullptr
		);
	}

	Destroy();
}

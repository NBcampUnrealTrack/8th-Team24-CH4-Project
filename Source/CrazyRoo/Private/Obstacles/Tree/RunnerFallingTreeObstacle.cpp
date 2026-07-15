// Copyright Epic Games, Inc. All Rights Reserved.

#include "Obstacles/Tree/RunnerFallingTreeObstacle.h"

#include "Audio/CrazyRooAudioNetManager.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Player/SharedRailCharacter.h"

ARunnerFallingTreeObstacle::ARunnerFallingTreeObstacle()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	TreePivot = CreateDefaultSubobject<USceneComponent>(TEXT("TreePivot"));
	TreePivot->SetupAttachment(SceneRoot);

	TreeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TreeMesh"));
	TreeMesh->SetupAttachment(TreePivot);
	TreeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(SceneRoot);
	TriggerBox->SetBoxExtent(FVector(250.0f, 250.0f, 200.0f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->SetGenerateOverlapEvents(true);

	DamageBox = CreateDefaultSubobject<UBoxComponent>(TEXT("DamageBox"));
	DamageBox->SetupAttachment(SceneRoot);
	DamageBox->SetBoxExtent(FVector(500.0f, 250.0f, 160.0f));
	DamageBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DamageBox->SetCollisionObjectType(ECC_WorldDynamic);
	DamageBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	DamageBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	DamageBox->SetGenerateOverlapEvents(false);

	FallenBlockingBox = CreateDefaultSubobject<UBoxComponent>(TEXT("FallenBlockingBox"));
	FallenBlockingBox->SetupAttachment(SceneRoot);
	FallenBlockingBox->SetBoxExtent(FVector(500.0f, 250.0f, 160.0f));
	FallenBlockingBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FallenBlockingBox->SetCollisionObjectType(ECC_WorldDynamic);
	FallenBlockingBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	FallenBlockingBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	FallenBlockingBox->SetGenerateOverlapEvents(false);
}

void ARunnerFallingTreeObstacle::BeginPlay()
{
	Super::BeginPlay();

	if (DamageBox)
	{
		DamageBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		DamageBox->SetGenerateOverlapEvents(false);
	}

	if (FallenBlockingBox)
	{
		FallenBlockingBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		FallenBlockingBox->SetGenerateOverlapEvents(false);
	}

	if (!HasAuthority())
	{
		return;
	}

	FallState.Phase = ERunnerTreeFallPhase::Standing;
	FallState.ServerStartTime = 0.0f;

	if (TriggerBox)
	{
		TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		TriggerBox->SetGenerateOverlapEvents(true);
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ARunnerFallingTreeObstacle::HandleTriggerBeginOverlap);
	}

	if (DamageBox)
	{
		DamageBox->OnComponentBeginOverlap.AddDynamic(this, &ARunnerFallingTreeObstacle::HandleDamageBoxBeginOverlap);
	}

	if (FallenBlockingBox)
	{
		FallenBlockingBox->OnComponentBeginOverlap.AddDynamic(this, &ARunnerFallingTreeObstacle::HandleFallenBlockingBoxBeginOverlap);
	}
}

void ARunnerFallingTreeObstacle::StartFalling()
{
	if (!HasAuthority())
	{
		return;
	}

	if (FallState.Phase != ERunnerTreeFallPhase::Standing)
	{
		return;
	}

	if (TriggerBox)
	{
		TriggerBox->SetGenerateOverlapEvents(false);
		TriggerBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	UE_LOG(LogTemp, Log, TEXT("[RunnerFallingTree] Request TreeFall SFX"));
	if (ACrazyRooAudioNetManager* AudioNetManager = ACrazyRooAudioNetManager::FindAudioNetManager(GetWorld()))
	{
		const FVector SoundLocation = TreePivot ? TreePivot->GetComponentLocation() : GetActorLocation();
		AudioNetManager->RequestPlaySFXAtLocation(ECrazyRooSFXType::TreeFall, SoundLocation);
	}

	FallState.Phase = ERunnerTreeFallPhase::Falling;
	FallState.ServerStartTime = GetServerTime();

	UE_LOG(LogTemp, Log, TEXT("[RunnerFallingTree] StartFalling"));
	OnRep_FallState();
	ForceNetUpdate();

	if (ImpactTime <= 0.0f)
	{
		HandleImpact();
		return;
	}

	GetWorldTimerManager().SetTimer(ImpactTimerHandle, this, &ARunnerFallingTreeObstacle::HandleImpact, ImpactTime, false);
}

void ARunnerFallingTreeObstacle::OnRep_FallState()
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	switch (FallState.Phase)
	{
	case ERunnerTreeFallPhase::Standing:
		BP_OnTreeResetToStanding();
		break;
	case ERunnerTreeFallPhase::Falling:
		BP_OnTreeFallStarted(FallState.ServerStartTime);
		break;
	case ERunnerTreeFallPhase::Impacted:
		BP_OnTreeImpacted();
		break;
	default:
		break;
	}
}

void ARunnerFallingTreeObstacle::HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority())
	{
		return;
	}

	if (FallState.Phase != ERunnerTreeFallPhase::Standing)
	{
		return;
	}

	if (!IsValidRunnerPlayer(OtherActor))
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[RunnerFallingTree] Triggered by player"));
	StartFalling();
}

void ARunnerFallingTreeObstacle::HandleDamageBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || FallState.Phase != ERunnerTreeFallPhase::Impacted)
	{
		return;
	}

	if (IsValidRunnerPlayer(OtherActor))
	{
		ApplyTreeHit(OtherActor);
	}
}

void ARunnerFallingTreeObstacle::HandleFallenBlockingBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || FallState.Phase != ERunnerTreeFallPhase::Impacted)
	{
		return;
	}

	if (IsValidRunnerPlayer(OtherActor))
	{
		ApplyTreeHit(OtherActor);
	}
}

void ARunnerFallingTreeObstacle::HandleImpact()
{
	if (!HasAuthority())
	{
		return;
	}

	if (FallState.Phase == ERunnerTreeFallPhase::Impacted)
	{
		return;
	}

	FallState.Phase = ERunnerTreeFallPhase::Impacted;
	UE_LOG(LogTemp, Log, TEXT("[RunnerFallingTree] Impact"));

	TArray<AActor*> OverlappingActors;
	if (DamageBox)
	{
		DamageBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		DamageBox->SetGenerateOverlapEvents(true);
		DamageBox->UpdateOverlaps();
		DamageBox->GetOverlappingActors(OverlappingActors);
		for (AActor* Actor : OverlappingActors)
		{
			ApplyTreeHit(Actor);
		}
	}

	OverlappingActors.Reset();
	if (FallenBlockingBox)
	{
		FallenBlockingBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		FallenBlockingBox->SetGenerateOverlapEvents(true);
		FallenBlockingBox->UpdateOverlaps();
		FallenBlockingBox->GetOverlappingActors(OverlappingActors);
		for (AActor* Actor : OverlappingActors)
		{
			ApplyTreeHit(Actor);
		}
	}

	OnRep_FallState();
	ForceNetUpdate();

	if (DamageActiveDuration <= 0.0f)
	{
		DisableDamageBox();
		return;
	}

	GetWorldTimerManager().SetTimer(DisableDamageBoxTimerHandle, this, &ARunnerFallingTreeObstacle::DisableDamageBox, DamageActiveDuration, false);
}

void ARunnerFallingTreeObstacle::DisableDamageBox()
{
	if (!HasAuthority())
	{
		return;
	}

	if (DamageBox)
	{
		DamageBox->SetGenerateOverlapEvents(false);
		DamageBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	UE_LOG(LogTemp, Log, TEXT("[RunnerFallingTree] DisableDamageBox"));
}

void ARunnerFallingTreeObstacle::ApplyTreeHit(AActor* Actor)
{
	if (!HasAuthority() || !IsValidRunnerPlayer(Actor))
	{
		return;
	}

	ASharedRailCharacter* SharedCharacter = Cast<ASharedRailCharacter>(Actor);
	if (!SharedCharacter && Actor && Actor->ActorHasTag(TEXT("Player")))
	{
		SharedCharacter = Cast<ASharedRailCharacter>(UGameplayStatics::GetActorOfClass(this, ASharedRailCharacter::StaticClass()));
	}

	if (!IsValid(SharedCharacter))
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunnerFallingTree] ApplyTreeHit failed: SharedRailCharacter not found"));
		return;
	}

	if (DamagedActors.Contains(SharedCharacter))
	{
		return;
	}

	DamagedActors.Add(SharedCharacter);
	UE_LOG(LogTemp, Log, TEXT("[RunnerFallingTree] ApplyTreeHit Damage=%.1f"), TreeDamage);
	SharedCharacter->ApplyPlayerDamage(TreeDamage, this, bIgnoreInvincibility);
}

bool ARunnerFallingTreeObstacle::IsValidRunnerPlayer(AActor* Actor) const
{
	if (!IsValid(Actor))
	{
		return false;
	}

	if (Cast<ASharedRailCharacter>(Actor))
	{
		return true;
	}

	return Actor->ActorHasTag(TEXT("Player"));
}

float ARunnerFallingTreeObstacle::GetServerTime() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0f;
	}

	if (const AGameStateBase* GameState = World->GetGameState())
	{
		return GameState->GetServerWorldTimeSeconds();
	}

	return World->GetTimeSeconds();
}

void ARunnerFallingTreeObstacle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARunnerFallingTreeObstacle, FallState);
}

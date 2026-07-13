// Copyright Epic Games, Inc. All Rights Reserved.

#include "Stage/RunnerFinishLine.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Enemies/FlyingEnemy.h"
#include "Enemies/FlyingEnemySpawner.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Player/SharedRailCharacter.h"
#include "Sound/SoundBase.h"

ARunnerFinishLine::ARunnerFinishLine()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	FinishLineBox = CreateDefaultSubobject<UBoxComponent>(TEXT("FinishLineBox"));
	FinishLineBox->SetupAttachment(SceneRoot);
	FinishLineBox->SetBoxExtent(FVector(200.0f, 450.0f, 200.0f));
	FinishLineBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	FinishLineBox->SetCollisionObjectType(ECC_WorldDynamic);
	FinishLineBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	FinishLineBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	FinishLineBox->SetGenerateOverlapEvents(true);

	LeftFireworkPoint = CreateDefaultSubobject<USceneComponent>(TEXT("LeftFireworkPoint"));
	LeftFireworkPoint->SetupAttachment(SceneRoot);

	RightFireworkPoint = CreateDefaultSubobject<USceneComponent>(TEXT("RightFireworkPoint"));
	RightFireworkPoint->SetupAttachment(SceneRoot);
}

void ARunnerFinishLine::BeginPlay()
{
	Super::BeginPlay();

	if (FinishLineBox)
	{
		FinishLineBox->OnComponentBeginOverlap.AddDynamic(this, &ARunnerFinishLine::HandleFinishLineBeginOverlap);
	}
}

void ARunnerFinishLine::HandleFinishLineBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !IsValidFinishTarget(OtherActor))
	{
		return;
	}

	if (bTriggerOnlyOnce && bHasTriggered)
	{
		return;
	}

	bHasTriggered = true;

	const FVector LeftLocation = LeftFireworkPoint ? LeftFireworkPoint->GetComponentLocation() : GetActorLocation();
	const FVector RightLocation = RightFireworkPoint ? RightFireworkPoint->GetComponentLocation() : GetActorLocation();
	StopEnemySystems();
	MulticastPlayFireworks(LeftLocation, RightLocation);
}

void ARunnerFinishLine::MulticastPlayFireworks_Implementation(FVector LeftLocation, FVector RightLocation)
{
	if (FireworkEffect)
	{
		if (UNiagaraComponent* LeftFirework = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, FireworkEffect, LeftLocation))
		{
			LeftFirework->SetWorldScale3D(FireworkScale);
		}

		if (UNiagaraComponent* RightFirework = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, FireworkEffect, RightLocation))
		{
			RightFirework->SetWorldScale3D(FireworkScale);
		}
	}

	if (FireworkSound)
	{
		const FVector SoundLocation = (LeftLocation + RightLocation) * 0.5f;
		UGameplayStatics::PlaySoundAtLocation(this, FireworkSound, SoundLocation, FireworkSoundVolume, FireworkSoundPitch);
	}
}

void ARunnerFinishLine::StopEnemySystems()
{
	TArray<AActor*> Spawners;
	UGameplayStatics::GetAllActorsOfClass(this, AFlyingEnemySpawner::StaticClass(), Spawners);
	for (AActor* Actor : Spawners)
	{
		if (AFlyingEnemySpawner* Spawner = Cast<AFlyingEnemySpawner>(Actor))
		{
			Spawner->StopSpawning();
		}
	}

	TArray<AActor*> FlyingEnemies;
	UGameplayStatics::GetAllActorsOfClass(this, AFlyingEnemy::StaticClass(), FlyingEnemies);
	for (AActor* Actor : FlyingEnemies)
	{
		if (AFlyingEnemy* Enemy = Cast<AFlyingEnemy>(Actor))
		{
			Enemy->DisableEnemyInteraction();
			Enemy->Destroy();
		}
	}
}

bool ARunnerFinishLine::IsValidFinishTarget(AActor* OtherActor) const
{
	return IsValid(OtherActor)
		&& (OtherActor->IsA<ASharedRailCharacter>() || OtherActor->ActorHasTag(TEXT("Player")));
}

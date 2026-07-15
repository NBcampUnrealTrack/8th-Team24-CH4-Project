// Copyright Epic Games, Inc. All Rights Reserved.

#include "Obstacles/Tree/RunnerTreeSpawnPoint.h"

#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"

ARunnerTreeSpawnPoint::ARunnerTreeSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	SpawnTransform = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnTransform"));
	SpawnTransform->SetupAttachment(SceneRoot);

	ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowComponent"));
	ArrowComponent->SetupAttachment(SpawnTransform);

	EditorBox = CreateDefaultSubobject<UBoxComponent>(TEXT("EditorBox"));
	EditorBox->SetupAttachment(SpawnTransform);
	EditorBox->SetBoxExtent(FVector(100.0f, 100.0f, 100.0f));
	EditorBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EditorBox->SetGenerateOverlapEvents(false);
}

ERunnerTreeSpawnSection ARunnerTreeSpawnPoint::GetSpawnSection() const
{
	return SpawnSection;
}

FTransform ARunnerTreeSpawnPoint::GetSpawnTransform() const
{
	return SpawnTransform ? SpawnTransform->GetComponentTransform() : GetActorTransform();
}

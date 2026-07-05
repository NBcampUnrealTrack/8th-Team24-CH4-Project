// Copyright Epic Games, Inc. All Rights Reserved.

#include "Obstacles/RunnerObstacleSpawner.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Obstacles/RunnerObstacle.h"

ARunnerObstacleSpawner::ARunnerObstacleSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	SpawnArea = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnArea"));
	SpawnArea->SetupAttachment(SceneRoot);
	SpawnArea->SetBoxExtent(FVector(150.0f, 450.0f, 100.0f));
	SpawnArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ARunnerObstacleSpawner::BeginPlay()
{
	Super::BeginPlay();

	// 장애물 생성은 서버 전용
	if (!HasAuthority())
	{
		return;
	}

	RandomStream.Initialize(bUseRandomSeed ? RandomSeed : FMath::Rand());

	if (bGenerateOnBeginPlay)
	{
		GenerateObstaclesOnce();
	}
}

void ARunnerObstacleSpawner::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void ARunnerObstacleSpawner::StartSpawning()
{
	if (!HasAuthority())
	{
		return;
	}

	GenerateObstaclesOnce();
}

void ARunnerObstacleSpawner::StopSpawning()
{
}

ARunnerObstacle* ARunnerObstacleSpawner::SpawnOnce()
{
	if (!HasAuthority())
	{
		return nullptr;
	}

	const FRunnerObstacleSpawnGroup* Group = ChooseSpawnGroup();
	if (!Group)
	{
		return nullptr;
	}

	const TSubclassOf<ARunnerObstacle> ObstacleClass = ChooseObstacleClass(*Group);
	if (!ObstacleClass)
	{
		return nullptr;
	}

	const ERunnerLane SpawnLane = ChooseLane(Group->LaneCoverage);
	const FVector SpawnLocation = GetSpawnLocation(SpawnLane);
	const FRotator SpawnRotation = GetActorRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;

	ARunnerObstacle* SpawnedObstacle = GetWorld()->SpawnActor<ARunnerObstacle>(ObstacleClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (SpawnedObstacle)
	{
		SpawnedObstacle->InitializeObstacle(Group->ActionType, Group->LaneCoverage, SpawnLane);
		OnObstacleSpawned.Broadcast(this, SpawnedObstacle);
	}

	return SpawnedObstacle;
}

int32 ARunnerObstacleSpawner::GenerateObstaclesOnce()
{
	if (!HasAuthority())
	{
		return 0;
	}

	if (bHasGenerated)
	{
		if (!bAllowRegenerate)
		{
			return 0;
		}

		ResetGeneratedObstacles(true);
	}

	int32 GeneratedCount = 0;
	TArray<ERunnerLane> AvailableSingleLanes = { ERunnerLane::Left, ERunnerLane::Center, ERunnerLane::Right };
	ERunnerLaneCoverage SelectedCoverage = ERunnerLaneCoverage::SingleLane;

	if (!ChooseGenerationCoverage(SelectedCoverage))
	{
		return 0;
	}

	if (SelectedCoverage == ERunnerLaneCoverage::AllLanes)
	{
		const int32 AllLaneCount = FMath::Clamp(MaxAllLaneObstacleCount, 0, 1);
		for (int32 Index = 0; Index < AllLaneCount; ++Index)
		{
			if (const FRunnerObstacleSpawnGroup* Group = ChooseSpawnGroupByCoverage(ERunnerLaneCoverage::AllLanes))
			{
				if (ARunnerObstacle* Obstacle = SpawnObstacleFromGroup(*Group, ERunnerLane::All))
				{
					GeneratedObstacles.Add(Obstacle);
					++GeneratedCount;
				}
			}
		}
	}
	else
	{
		const int32 SingleLaneCount = FMath::Clamp(MaxSingleLaneObstacleCount, 0, 2);
		for (int32 Index = 0; Index < SingleLaneCount; ++Index)
		{
			ERunnerLane SpawnLane = ERunnerLane::Center;
			if (!ChooseUnusedSingleLane(AvailableSingleLanes, SpawnLane))
			{
				break;
			}

			if (const FRunnerObstacleSpawnGroup* Group = ChooseSpawnGroupByCoverage(ERunnerLaneCoverage::SingleLane))
			{
				if (ARunnerObstacle* Obstacle = SpawnObstacleFromGroup(*Group, SpawnLane))
				{
					GeneratedObstacles.Add(Obstacle);
					++GeneratedCount;
				}
			}
		}
	}

	bHasGenerated = true;
	return GeneratedCount;
}

void ARunnerObstacleSpawner::ResetGeneratedObstacles(bool bDestroyExisting)
{
	if (!HasAuthority())
	{
		return;
	}

	if (bDestroyExisting)
	{
		for (const TWeakObjectPtr<ARunnerObstacle>& Obstacle : GeneratedObstacles)
		{
			if (Obstacle.IsValid())
			{
				Obstacle->Destroy();
			}
		}
	}

	GeneratedObstacles.Reset();
	bHasGenerated = false;
}

bool ARunnerObstacleSpawner::HasValidSpawnGroup() const
{
	for (const FRunnerObstacleSpawnGroup& Group : SpawnGroups)
	{
		if (Group.Weight > 0.0f && !Group.ObstacleClasses.IsEmpty())
		{
			return true;
		}
	}

	return false;
}

const FRunnerObstacleSpawnGroup* ARunnerObstacleSpawner::ChooseSpawnGroup()
{
	float TotalWeight = 0.0f;
	for (const FRunnerObstacleSpawnGroup& Group : SpawnGroups)
	{
		if (Group.Weight > 0.0f && !Group.ObstacleClasses.IsEmpty())
		{
			TotalWeight += Group.Weight;
		}
	}

	if (TotalWeight <= 0.0f)
	{
		return nullptr;
	}

	float Pick = GetRandomRange(0.0f, TotalWeight);
	for (const FRunnerObstacleSpawnGroup& Group : SpawnGroups)
	{
		if (Group.Weight <= 0.0f || Group.ObstacleClasses.IsEmpty())
		{
			continue;
		}

		Pick -= Group.Weight;
		if (Pick <= 0.0f)
		{
			return &Group;
		}
	}

	return nullptr;
}

const FRunnerObstacleSpawnGroup* ARunnerObstacleSpawner::ChooseSpawnGroupByCoverage(ERunnerLaneCoverage DesiredCoverage)
{
	float TotalWeight = 0.0f;
	for (const FRunnerObstacleSpawnGroup& Group : SpawnGroups)
	{
		if (Group.LaneCoverage == DesiredCoverage && Group.Weight > 0.0f && !Group.ObstacleClasses.IsEmpty())
		{
			TotalWeight += Group.Weight;
		}
	}

	if (TotalWeight <= 0.0f)
	{
		return nullptr;
	}

	float Pick = GetRandomRange(0.0f, TotalWeight);
	for (const FRunnerObstacleSpawnGroup& Group : SpawnGroups)
	{
		if (Group.LaneCoverage != DesiredCoverage || Group.Weight <= 0.0f || Group.ObstacleClasses.IsEmpty())
		{
			continue;
		}

		Pick -= Group.Weight;
		if (Pick <= 0.0f)
		{
			return &Group;
		}
	}

	return nullptr;
}

bool ARunnerObstacleSpawner::ChooseGenerationCoverage(ERunnerLaneCoverage& OutCoverage)
{
	const float AllLaneWeight = MaxAllLaneObstacleCount > 0 ? GetTotalWeightByCoverage(ERunnerLaneCoverage::AllLanes) : 0.0f;
	const float SingleLaneWeight = MaxSingleLaneObstacleCount > 0 ? GetTotalWeightByCoverage(ERunnerLaneCoverage::SingleLane) : 0.0f;
	const float TotalWeight = AllLaneWeight + SingleLaneWeight;

	if (TotalWeight <= 0.0f)
	{
		return false;
	}

	if (AllLaneWeight <= 0.0f)
	{
		OutCoverage = ERunnerLaneCoverage::SingleLane;
		return true;
	}

	if (SingleLaneWeight <= 0.0f)
	{
		OutCoverage = ERunnerLaneCoverage::AllLanes;
		return true;
	}

	OutCoverage = GetRandomRange(0.0f, TotalWeight) <= AllLaneWeight ? ERunnerLaneCoverage::AllLanes : ERunnerLaneCoverage::SingleLane;
	return true;
}

float ARunnerObstacleSpawner::GetTotalWeightByCoverage(ERunnerLaneCoverage DesiredCoverage) const
{
	float TotalWeight = 0.0f;
	for (const FRunnerObstacleSpawnGroup& Group : SpawnGroups)
	{
		if (Group.LaneCoverage == DesiredCoverage && Group.Weight > 0.0f && !Group.ObstacleClasses.IsEmpty())
		{
			TotalWeight += Group.Weight;
		}
	}

	return TotalWeight;
}

TSubclassOf<ARunnerObstacle> ARunnerObstacleSpawner::ChooseObstacleClass(const FRunnerObstacleSpawnGroup& Group)
{
	if (Group.ObstacleClasses.IsEmpty())
	{
		return nullptr;
	}

	return Group.ObstacleClasses[GetRandomIndex(Group.ObstacleClasses.Num())];
}

ERunnerLane ARunnerObstacleSpawner::ChooseLane(ERunnerLaneCoverage LaneCoverage)
{
	if (LaneCoverage == ERunnerLaneCoverage::AllLanes)
	{
		return ERunnerLane::All;
	}

	switch (GetRandomIndex(3))
	{
	case 0:
		return ERunnerLane::Left;
	case 1:
		return ERunnerLane::Center;
	default:
		return ERunnerLane::Right;
	}
}

bool ARunnerObstacleSpawner::ChooseUnusedSingleLane(TArray<ERunnerLane>& AvailableLanes, ERunnerLane& OutLane)
{
	if (AvailableLanes.IsEmpty())
	{
		return false;
	}

	const int32 LaneIndex = GetRandomIndex(AvailableLanes.Num());
	OutLane = AvailableLanes[LaneIndex];
	AvailableLanes.RemoveAt(LaneIndex);
	return true;
}

FVector ARunnerObstacleSpawner::GetSpawnLocation(ERunnerLane Lane)
{
	float LaneOffset = 0.0f;
	if (Lane == ERunnerLane::Left)
	{
		LaneOffset = -LaneSpacing;
	}
	else if (Lane == ERunnerLane::Right)
	{
		LaneOffset = LaneSpacing;
	}

	const FVector Extent = SpawnArea->GetScaledBoxExtent();
	const FVector LocalOffset(GetRandomRange(-Extent.X, Extent.X), LaneOffset, GetRandomRange(-Extent.Z, Extent.Z));
	return SpawnArea->GetComponentTransform().TransformPosition(LocalOffset);
}

ARunnerObstacle* ARunnerObstacleSpawner::SpawnObstacleFromGroup(const FRunnerObstacleSpawnGroup& Group, ERunnerLane SpawnLane)
{
	const TSubclassOf<ARunnerObstacle> ObstacleClass = ChooseObstacleClass(Group);
	if (!ObstacleClass || !GetWorld())
	{
		return nullptr;
	}

	const FVector SpawnLocation = GetSpawnLocation(SpawnLane);
	const FRotator SpawnRotation = GetActorRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;

	ARunnerObstacle* SpawnedObstacle = GetWorld()->SpawnActor<ARunnerObstacle>(ObstacleClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (SpawnedObstacle)
	{
		SpawnedObstacle->InitializeObstacle(Group.ActionType, Group.LaneCoverage, SpawnLane);
		OnObstacleSpawned.Broadcast(this, SpawnedObstacle);
	}

	return SpawnedObstacle;
}

float ARunnerObstacleSpawner::GetRandomRange(float Min, float Max)
{
	return bUseRandomSeed ? RandomStream.FRandRange(Min, Max) : FMath::FRandRange(Min, Max);
}

int32 ARunnerObstacleSpawner::GetRandomIndex(int32 Num)
{
	return Num > 0 ? (bUseRandomSeed ? RandomStream.RandRange(0, Num - 1) : FMath::RandRange(0, Num - 1)) : 0;
}

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunnerTypes.generated.h"

class AFlyingEnemy;
class ARunnerObstacle;

UENUM(BlueprintType)
enum class ERunnerObstacleActionType : uint8
{
	LaneDodge UMETA(DisplayName = "Lane Dodge"),
	JumpRequired UMETA(DisplayName = "Jump Required"),
	SlideRequired UMETA(DisplayName = "Slide Required"),
	ShootRequired UMETA(DisplayName = "Shoot Required")
};

UENUM(BlueprintType)
enum class ERunnerLaneCoverage : uint8
{
	SingleLane UMETA(DisplayName = "Single Lane"),
	AllLanes UMETA(DisplayName = "All Lanes")
};

UENUM(BlueprintType)
enum class ERunnerLane : uint8
{
	Left UMETA(DisplayName = "Left"),
	Center UMETA(DisplayName = "Center"),
	Right UMETA(DisplayName = "Right"),
	All UMETA(DisplayName = "All")
};

UENUM(BlueprintType)
enum class ERunnerTurnDirection : uint8
{
	LeftTurn UMETA(DisplayName = "Left Turn"),
	RightTurn UMETA(DisplayName = "Right Turn")
};

UENUM(BlueprintType)
enum class ERunnerTurnInputDirection : uint8
{
	LeftInput UMETA(DisplayName = "Left Input"),
	RightInput UMETA(DisplayName = "Right Input")
};

UENUM(BlueprintType)
enum class ERunnerTurnJudgeMode : uint8
{
	ForcedTurn UMETA(DisplayName = "Forced Turn"),
	BranchTurn UMETA(DisplayName = "Branch Turn")
};

UENUM(BlueprintType)
enum class EFlyingEnemySpawnSide : uint8
{
	RandomSide UMETA(DisplayName = "Random Side"),
	LeftSide UMETA(DisplayName = "Left Side"),
	RightSide UMETA(DisplayName = "Right Side")
};

UENUM(BlueprintType)
enum class EFlyingEnemyMoveMode : uint8
{
	FixedDirection UMETA(DisplayName = "Fixed Direction"),
	TowardTarget UMETA(DisplayName = "Toward Target")
};

UENUM(BlueprintType)
enum class EFlyingEnemySlot : uint8
{
	Left UMETA(DisplayName = "Left"),
	Center UMETA(DisplayName = "Center"),
	Right UMETA(DisplayName = "Right")
};

USTRUCT(BlueprintType)
struct FRunnerObstacleSpawnGroup
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	ERunnerObstacleActionType ActionType = ERunnerObstacleActionType::LaneDodge;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	ERunnerLaneCoverage LaneCoverage = ERunnerLaneCoverage::SingleLane;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	TArray<TSubclassOf<ARunnerObstacle>> ObstacleClasses;
};

USTRUCT(BlueprintType)
struct FFlyingEnemySpawnGroup
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	TArray<TSubclassOf<AFlyingEnemy>> EnemyClasses;
};

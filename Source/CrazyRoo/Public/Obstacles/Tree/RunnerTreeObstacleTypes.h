// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunnerTreeObstacleTypes.generated.h"

UENUM(BlueprintType)
enum class ERunnerTreeSpawnSection : uint8
{
	Early UMETA(DisplayName = "Early"),
	Late UMETA(DisplayName = "Late")
};

UENUM(BlueprintType)
enum class ERunnerTreeFallPhase : uint8
{
	Standing UMETA(DisplayName = "Standing"),
	Falling UMETA(DisplayName = "Falling"),
	Impacted UMETA(DisplayName = "Impacted")
};

USTRUCT(BlueprintType)
struct CRAZYROO_API FRunnerTreeFallRepData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	ERunnerTreeFallPhase Phase = ERunnerTreeFallPhase::Standing;

	UPROPERTY(BlueprintReadOnly)
	float ServerStartTime = 0.0f;
};

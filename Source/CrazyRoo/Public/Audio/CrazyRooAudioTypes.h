// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CrazyRooAudioTypes.generated.h"

class USoundBase;

UENUM(BlueprintType)
enum class ECrazyRooBGMType : uint8
{
	None UMETA(DisplayName = "None"),
	MainMenu UMETA(DisplayName = "Main Menu"),
	SelectMenu UMETA(DisplayName = "Select Menu"),
	InGame UMETA(DisplayName = "In Game"),
	StageClear UMETA(DisplayName = "Stage Clear"),
	Death UMETA(DisplayName = "Death")
};

UENUM(BlueprintType)
enum class ECrazyRooSFXType : uint8
{
	None UMETA(DisplayName = "None"),
	FinishFirework UMETA(DisplayName = "Finish Firework"),
	ObstacleDestroy UMETA(DisplayName = "Obstacle Destroy"),
	EnemyHit UMETA(DisplayName = "Enemy Hit"),
	EnemyAttack UMETA(DisplayName = "Enemy Attack"),
	CageClose UMETA(DisplayName = "Cage Close"),
	UIButtonClick UMETA(DisplayName = "UI Button Click"),
	TreeFall UMETA(DisplayName = "Tree Fall")
};

USTRUCT(BlueprintType)
struct CRAZYROO_API FCrazyRooBGMDataRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECrazyRooBGMType BGMType = ECrazyRooBGMType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<USoundBase> Sound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
	float VolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", Units = "s"))
	float FadeInTime = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", Units = "s"))
	float FadeOutTime = 0.25f;
};

USTRUCT(BlueprintType)
struct CRAZYROO_API FCrazyRooSFXDataRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECrazyRooSFXType SFXType = ECrazyRooSFXType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<USoundBase> Sound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
	float VolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
	float PitchMultiplier = 1.0f;
};

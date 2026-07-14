// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Audio/CrazyRooAudioTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CrazyRooAudioSubsystem.generated.h"

class UAudioComponent;
class UDataTable;

UCLASS()
class CRAZYROO_API UCrazyRooAudioSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "CrazyRoo|Audio")
	void SetAudioDataTables(UDataTable* InBGMTable, UDataTable* InSFXTable);

	UFUNCTION(BlueprintCallable, Category = "CrazyRoo|Audio")
	void PlayBGM(ECrazyRooBGMType BGMType);

	UFUNCTION(BlueprintCallable, Category = "CrazyRoo|Audio")
	void StopBGM(float FadeOutTime = 0.25f);

	UFUNCTION(BlueprintCallable, Category = "CrazyRoo|Audio")
	void SetBGMVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "CrazyRoo|Audio")
	void SetSFXVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "CrazyRoo|Audio")
	void PlayLocalSFX(ECrazyRooSFXType SFXType);

	UFUNCTION(BlueprintCallable, Category = "CrazyRoo|Audio")
	void PlayLocalSFXAtLocation(ECrazyRooSFXType SFXType, FVector Location);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CrazyRoo|Audio")
	TObjectPtr<UDataTable> BGMDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CrazyRoo|Audio")
	TObjectPtr<UDataTable> SFXDataTable;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> CurrentBGMComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CrazyRoo|Audio")
	ECrazyRooBGMType CurrentBGMType = ECrazyRooBGMType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CrazyRoo|Audio")
	float BGMVolume = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CrazyRoo|Audio")
	float SFXVolume = 1.0f;

	float CurrentBGMDataVolumeMultiplier = 1.0f;

	const FCrazyRooBGMDataRow* FindBGMData(ECrazyRooBGMType BGMType) const;
	const FCrazyRooSFXDataRow* FindSFXData(ECrazyRooSFXType SFXType) const;
	bool ShouldSkipAudioPlayback() const;
};

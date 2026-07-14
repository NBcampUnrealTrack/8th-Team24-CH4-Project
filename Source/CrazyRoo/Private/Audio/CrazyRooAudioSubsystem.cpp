// Copyright Epic Games, Inc. All Rights Reserved.

#include "Audio/CrazyRooAudioSubsystem.h"

#include "Components/AudioComponent.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

namespace
{
FString GetBGMTypeName(ECrazyRooBGMType BGMType)
{
	if (const UEnum* Enum = StaticEnum<ECrazyRooBGMType>())
	{
		return Enum->GetNameStringByValue(static_cast<int64>(BGMType));
	}

	return TEXT("Unknown");
}

FString GetSFXTypeName(ECrazyRooSFXType SFXType)
{
	if (const UEnum* Enum = StaticEnum<ECrazyRooSFXType>())
	{
		return Enum->GetNameStringByValue(static_cast<int64>(SFXType));
	}

	return TEXT("Unknown");
}
}

void UCrazyRooAudioSubsystem::SetAudioDataTables(UDataTable* InBGMTable, UDataTable* InSFXTable)
{
	BGMDataTable = InBGMTable;
	SFXDataTable = InSFXTable;
	UE_LOG(LogTemp, Log, TEXT("[AudioSubsystem] SetAudioDataTables: BGM=%s SFX=%s"), *GetNameSafe(BGMDataTable.Get()), *GetNameSafe(SFXDataTable.Get()));
}

void UCrazyRooAudioSubsystem::PlayBGM(ECrazyRooBGMType BGMType)
{
	UE_LOG(LogTemp, Log, TEXT("[AudioSubsystem] PlayBGM: %s"), *GetBGMTypeName(BGMType));

	if (ShouldSkipAudioPlayback())
	{
		return;
	}

	if (BGMType == ECrazyRooBGMType::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AudioSubsystem] Missing BGM Data: None"));
		return;
	}

	const FCrazyRooBGMDataRow* BGMData = FindBGMData(BGMType);
	if (!BGMData || !BGMData->Sound)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AudioSubsystem] Missing BGM Data: %s"), *GetBGMTypeName(BGMType));
		return;
	}

	if (CurrentBGMComponent)
	{
		const float FadeOutTime = FMath::Max(0.0f, BGMData->FadeOutTime);
		if (FadeOutTime > 0.0f)
		{
			CurrentBGMComponent->FadeOut(FadeOutTime, 0.0f);
		}
		else
		{
			CurrentBGMComponent->Stop();
		}
		CurrentBGMComponent = nullptr;
		CurrentBGMType = ECrazyRooBGMType::None;
	}

	const float FinalVolume = BGMVolume * FMath::Max(0.0f, BGMData->VolumeMultiplier);
	CurrentBGMDataVolumeMultiplier = FMath::Max(0.0f, BGMData->VolumeMultiplier);
	CurrentBGMComponent = UGameplayStatics::CreateSound2D(this, BGMData->Sound, FinalVolume, 1.0f, 0.0f, nullptr, false, false);
	if (!CurrentBGMComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AudioSubsystem] Failed to create BGM component: %s"), *GetBGMTypeName(BGMType));
		return;
	}

	CurrentBGMType = BGMType;
	if (BGMData->FadeInTime > 0.0f)
	{
		CurrentBGMComponent->FadeIn(BGMData->FadeInTime, FinalVolume);
	}
	else
	{
		CurrentBGMComponent->Play();
	}
}

void UCrazyRooAudioSubsystem::StopBGM(float FadeOutTime)
{
	UE_LOG(LogTemp, Log, TEXT("[AudioSubsystem] StopBGM"));

	if (!CurrentBGMComponent)
	{
		CurrentBGMType = ECrazyRooBGMType::None;
		return;
	}

	const float SafeFadeOutTime = FMath::Max(0.0f, FadeOutTime);
	if (SafeFadeOutTime > 0.0f)
	{
		CurrentBGMComponent->FadeOut(SafeFadeOutTime, 0.0f);
	}
	else
	{
		CurrentBGMComponent->Stop();
	}

	CurrentBGMComponent = nullptr;
	CurrentBGMType = ECrazyRooBGMType::None;
	CurrentBGMDataVolumeMultiplier = 1.0f;
}

void UCrazyRooAudioSubsystem::SetBGMVolume(float Volume)
{
	BGMVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	UE_LOG(LogTemp, Log, TEXT("[AudioSubsystem] SetBGMVolume: %.2f"), BGMVolume);

	if (CurrentBGMComponent)
	{
		CurrentBGMComponent->SetVolumeMultiplier(BGMVolume * CurrentBGMDataVolumeMultiplier);
	}
}

void UCrazyRooAudioSubsystem::SetSFXVolume(float Volume)
{
	SFXVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	UE_LOG(LogTemp, Log, TEXT("[AudioSubsystem] SetSFXVolume: %.2f"), SFXVolume);
}

void UCrazyRooAudioSubsystem::PlayLocalSFX(ECrazyRooSFXType SFXType)
{
	UE_LOG(LogTemp, Log, TEXT("[AudioSubsystem] PlaySFX: %s"), *GetSFXTypeName(SFXType));

	if (ShouldSkipAudioPlayback())
	{
		return;
	}

	const FCrazyRooSFXDataRow* SFXData = FindSFXData(SFXType);
	if (SFXType == ECrazyRooSFXType::None || !SFXData || !SFXData->Sound)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AudioSubsystem] Missing SFX Data: %s"), *GetSFXTypeName(SFXType));
		return;
	}

	UGameplayStatics::PlaySound2D(this, SFXData->Sound, SFXVolume * FMath::Max(0.0f, SFXData->VolumeMultiplier), FMath::Max(0.0f, SFXData->PitchMultiplier));
}

void UCrazyRooAudioSubsystem::PlayLocalSFXAtLocation(ECrazyRooSFXType SFXType, FVector Location)
{
	UE_LOG(LogTemp, Log, TEXT("[AudioSubsystem] PlaySFXAtLocation: %s"), *GetSFXTypeName(SFXType));

	if (ShouldSkipAudioPlayback())
	{
		return;
	}

	const FCrazyRooSFXDataRow* SFXData = FindSFXData(SFXType);
	if (SFXType == ECrazyRooSFXType::None || !SFXData || !SFXData->Sound)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AudioSubsystem] Missing SFX Data: %s"), *GetSFXTypeName(SFXType));
		return;
	}

	UGameplayStatics::PlaySoundAtLocation(this, SFXData->Sound, Location, SFXVolume * FMath::Max(0.0f, SFXData->VolumeMultiplier), FMath::Max(0.0f, SFXData->PitchMultiplier));
}

const FCrazyRooBGMDataRow* UCrazyRooAudioSubsystem::FindBGMData(ECrazyRooBGMType BGMType) const
{
	if (!BGMDataTable)
	{
		return nullptr;
	}

	for (const TPair<FName, uint8*>& RowPair : BGMDataTable->GetRowMap())
	{
		const FCrazyRooBGMDataRow* Row = reinterpret_cast<const FCrazyRooBGMDataRow*>(RowPair.Value);
		if (Row && Row->BGMType == BGMType)
		{
			return Row;
		}
	}

	return nullptr;
}

const FCrazyRooSFXDataRow* UCrazyRooAudioSubsystem::FindSFXData(ECrazyRooSFXType SFXType) const
{
	if (!SFXDataTable)
	{
		return nullptr;
	}

	for (const TPair<FName, uint8*>& RowPair : SFXDataTable->GetRowMap())
	{
		const FCrazyRooSFXDataRow* Row = reinterpret_cast<const FCrazyRooSFXDataRow*>(RowPair.Value);
		if (Row && Row->SFXType == SFXType)
		{
			return Row;
		}
	}

	return nullptr;
}

bool UCrazyRooAudioSubsystem::ShouldSkipAudioPlayback() const
{
	const UWorld* World = GetWorld();
	return World && World->GetNetMode() == NM_DedicatedServer;
}

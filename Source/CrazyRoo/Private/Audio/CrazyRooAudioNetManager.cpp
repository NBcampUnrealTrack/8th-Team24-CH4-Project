// Copyright Epic Games, Inc. All Rights Reserved.

#include "Audio/CrazyRooAudioNetManager.h"

#include "Audio/CrazyRooAudioSubsystem.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"

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

ACrazyRooAudioNetManager::ACrazyRooAudioNetManager()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;
}

void ACrazyRooAudioNetManager::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("[AudioNetManager] BeginPlay Register Audio DataTables: BGM=%s SFX=%s"), *GetNameSafe(BGMDataTable.Get()), *GetNameSafe(SFXDataTable.Get()));

	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AudioNetManager] Failed to register audio data tables: GameInstance is null"));
		return;
	}

	UCrazyRooAudioSubsystem* AudioSubsystem = GameInstance->GetSubsystem<UCrazyRooAudioSubsystem>();
	if (!AudioSubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AudioNetManager] Failed to register audio data tables: AudioSubsystem is null"));
		return;
	}

	AudioSubsystem->SetAudioDataTables(BGMDataTable.Get(), SFXDataTable.Get());
}

void ACrazyRooAudioNetManager::RequestPlayBGM(ECrazyRooBGMType BGMType)
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[AudioNetManager] RequestPlayBGM ignored on non-authority: %s"), *GetBGMTypeName(BGMType));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[AudioNetManager] RequestPlayBGM: %s"), *GetBGMTypeName(BGMType));
	MulticastPlayBGM(BGMType);
}

void ACrazyRooAudioNetManager::RequestPlaySFXAtLocation(ECrazyRooSFXType SFXType, FVector Location)
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[AudioNetManager] RequestPlaySFXAtLocation ignored on non-authority: %s"), *GetSFXTypeName(SFXType));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[AudioNetManager] RequestPlaySFXAtLocation: %s"), *GetSFXTypeName(SFXType));
	MulticastPlaySFXAtLocation(SFXType, Location);
}

void ACrazyRooAudioNetManager::MulticastPlayBGM_Implementation(ECrazyRooBGMType BGMType)
{
	UE_LOG(LogTemp, Log, TEXT("[AudioNetManager] MulticastPlayBGM: %s"), *GetBGMTypeName(BGMType));

	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UCrazyRooAudioSubsystem* AudioSubsystem = GameInstance->GetSubsystem<UCrazyRooAudioSubsystem>())
		{
			AudioSubsystem->PlayBGM(BGMType);
		}
	}
}

void ACrazyRooAudioNetManager::MulticastPlaySFXAtLocation_Implementation(ECrazyRooSFXType SFXType, FVector Location)
{
	UE_LOG(LogTemp, Log, TEXT("[AudioNetManager] MulticastPlaySFXAtLocation: %s"), *GetSFXTypeName(SFXType));

	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UCrazyRooAudioSubsystem* AudioSubsystem = GameInstance->GetSubsystem<UCrazyRooAudioSubsystem>())
		{
			AudioSubsystem->PlayLocalSFXAtLocation(SFXType, Location);
		}
	}
}

ACrazyRooAudioNetManager* ACrazyRooAudioNetManager::FindAudioNetManager(UWorld* World)
{
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AudioNetManager] FindAudioNetManager failed: World is null"));
		return nullptr;
	}

	for (TActorIterator<ACrazyRooAudioNetManager> It(World); It; ++It)
	{
		return *It;
	}

	UE_LOG(LogTemp, Warning, TEXT("[AudioNetManager] FindAudioNetManager failed: manager not found"));
	return nullptr;
}

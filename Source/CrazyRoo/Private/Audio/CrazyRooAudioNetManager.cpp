// Copyright Epic Games, Inc. All Rights Reserved.

#include "Audio/CrazyRooAudioNetManager.h"

#include "Audio/CrazyRooAudioSubsystem.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"

namespace
{
FString GetAudioNetBGMTypeName(ECrazyRooBGMType BGMType)
{
	if (const UEnum* Enum = StaticEnum<ECrazyRooBGMType>())
	{
		return Enum->GetNameStringByValue(static_cast<int64>(BGMType));
	}

	return TEXT("Unknown");
}

FString GetAudioNetSFXTypeName(ECrazyRooSFXType SFXType)
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

	EnsureAudioTablesRegistered();
	if (CurrentBGMType != ECrazyRooBGMType::None)
	{
		PlayLocalBGM(CurrentBGMType);
	}
}

void ACrazyRooAudioNetManager::RequestPlayBGM(ECrazyRooBGMType BGMType)
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[AudioNetManager] RequestPlayBGM ignored on non-authority: %s"), *GetAudioNetBGMTypeName(BGMType));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[AudioNetManager] RequestPlayBGM Set CurrentBGMType: %s"), *GetAudioNetBGMTypeName(BGMType));
	CurrentBGMType = BGMType;
	ForceNetUpdate();
	PlayLocalBGM(BGMType);
}

void ACrazyRooAudioNetManager::RequestStopBGM(float FadeOutTime)
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[AudioNetManager] RequestStopBGM ignored on non-authority"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[AudioNetManager] RequestStopBGM: %.2f"), FadeOutTime);
	CurrentBGMType = ECrazyRooBGMType::None;
	ForceNetUpdate();
	MulticastStopBGM(FadeOutTime);
}

void ACrazyRooAudioNetManager::RequestPlaySFXAtLocation(ECrazyRooSFXType SFXType, FVector Location)
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[AudioNetManager] RequestPlaySFXAtLocation ignored on non-authority: %s"), *GetAudioNetSFXTypeName(SFXType));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[AudioNetManager] RequestPlaySFXAtLocation: %s"), *GetAudioNetSFXTypeName(SFXType));
	MulticastPlaySFXAtLocation(SFXType, Location);
}

void ACrazyRooAudioNetManager::MulticastStopBGM_Implementation(float FadeOutTime)
{
	UE_LOG(LogTemp, Log, TEXT("[AudioNetManager] MulticastStopBGM: %.2f"), FadeOutTime);
	StopLocalBGM(FadeOutTime);
}

void ACrazyRooAudioNetManager::MulticastPlaySFXAtLocation_Implementation(ECrazyRooSFXType SFXType, FVector Location)
{
	UE_LOG(LogTemp, Log, TEXT("[AudioNetManager] MulticastPlaySFXAtLocation: %s"), *GetAudioNetSFXTypeName(SFXType));

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

void ACrazyRooAudioNetManager::OnRep_CurrentBGMType()
{
	UE_LOG(LogTemp, Log, TEXT("[AudioNetManager] OnRep_CurrentBGMType: %s"), *GetAudioNetBGMTypeName(CurrentBGMType));

	if (CurrentBGMType == ECrazyRooBGMType::None)
	{
		return;
	}

	PlayLocalBGM(CurrentBGMType);
}

UCrazyRooAudioSubsystem* ACrazyRooAudioNetManager::EnsureAudioTablesRegistered()
{
	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AudioNetManager] Failed to register audio data tables: GameInstance is null"));
		return nullptr;
	}

	UCrazyRooAudioSubsystem* AudioSubsystem = GameInstance->GetSubsystem<UCrazyRooAudioSubsystem>();
	if (!AudioSubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AudioNetManager] Failed to register audio data tables: AudioSubsystem is null"));
		return nullptr;
	}

	AudioSubsystem->SetAudioDataTables(BGMDataTable.Get(), SFXDataTable.Get());
	return AudioSubsystem;
}

void ACrazyRooAudioNetManager::PlayLocalBGM(ECrazyRooBGMType BGMType)
{
	if (BGMType == ECrazyRooBGMType::None)
	{
		return;
	}

	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[AudioNetManager] PlayLocalBGM: %s"), *GetAudioNetBGMTypeName(BGMType));
	if (UCrazyRooAudioSubsystem* AudioSubsystem = EnsureAudioTablesRegistered())
	{
		AudioSubsystem->PlayBGM(BGMType);
	}
}

void ACrazyRooAudioNetManager::StopLocalBGM(float FadeOutTime)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[AudioNetManager] StopLocalBGM: %.2f"), FadeOutTime);
	if (UCrazyRooAudioSubsystem* AudioSubsystem = EnsureAudioTablesRegistered())
	{
		AudioSubsystem->StopBGM(FadeOutTime);
	}
}

void ACrazyRooAudioNetManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACrazyRooAudioNetManager, CurrentBGMType);
}

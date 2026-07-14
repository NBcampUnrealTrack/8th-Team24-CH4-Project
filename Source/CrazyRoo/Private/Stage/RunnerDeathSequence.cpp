// Copyright Epic Games, Inc. All Rights Reserved.

#include "Stage/RunnerDeathSequence.h"

#include "Audio/CrazyRooAudioNetManager.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Enemies/FlyingEnemy.h"
#include "Enemies/FlyingEnemySpawner.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Manager/SharedCharacterManager.h"
#include "Player/SharedRailCharacter.h"

ARunnerDeathSequence::ARunnerDeathSequence()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	CageSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("CageSpawnPoint"));
	CageSpawnPoint->SetupAttachment(SceneRoot);

	DeathCameraPoint = CreateDefaultSubobject<USceneComponent>(TEXT("DeathCameraPoint"));
	DeathCameraPoint->SetupAttachment(SceneRoot);
}

void ARunnerDeathSequence::StartDeathSequence(ASharedRailCharacter* DeadCharacter)
{
	if (!HasAuthority())
	{
		return;
	}

	if (bTriggerOnlyOnce && bHasTriggered)
	{
		return;
	}

	if (!IsValid(DeadCharacter))
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunnerDeathSequence] StartDeathSequence failed: DeadCharacter is invalid"));
		return;
	}

	bHasTriggered = true;
	UE_LOG(LogTemp, Log, TEXT("[RunnerDeathSequence] StartDeathSequence"));

	const FTransform CageSpawnTransform = CageSpawnPoint ? CageSpawnPoint->GetComponentTransform() : GetActorTransform();
	const FTransform DeathCameraTransform = DeathCameraPoint ? DeathCameraPoint->GetComponentTransform() : GetActorTransform();

	RequestDeathAudio(CageSpawnTransform.GetLocation());
	StopRunnerSystems(DeadCharacter);
	HideSharedCharacter(DeadCharacter);
	MulticastPlayDeathSequence(CageSpawnTransform, DeathCameraTransform);
}

void ARunnerDeathSequence::MulticastPlayDeathSequence_Implementation(FTransform CageSpawnTransform, FTransform DeathCameraTransform)
{
	UE_LOG(LogTemp, Log, TEXT("[RunnerDeathSequence] MulticastPlayDeathSequence"));

	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	PlayLocalDeathSequence(CageSpawnTransform, DeathCameraTransform);
}

void ARunnerDeathSequence::StopRunnerSystems(ASharedRailCharacter* DeadCharacter)
{
	if (ASharedCharacterManager* SharedCharacterManager = Cast<ASharedCharacterManager>(UGameplayStatics::GetActorOfClass(this, ASharedCharacterManager::StaticClass())))
	{
		SharedCharacterManager->StopMove();
		SharedCharacterManager->SetRunnerInputLocked(true);
	}

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

void ARunnerDeathSequence::HideSharedCharacter(ASharedRailCharacter* DeadCharacter) const
{
	if (!IsValid(DeadCharacter))
	{
		return;
	}

	DeadCharacter->SetActorHiddenInGame(true);
	DeadCharacter->SetActorEnableCollision(false);
}

void ARunnerDeathSequence::PlayLocalDeathSequence(const FTransform& /*CageSpawnTransform*/, const FTransform& DeathCameraTransform)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UClass* CameraClassToSpawn = DeathCameraClass ? DeathCameraClass.Get() : ACameraActor::StaticClass();
	ACameraActor* DeathCamera = World->SpawnActor<ACameraActor>(CameraClassToSpawn, DeathCameraTransform);

	if (DeathCamera && DeathCamera->GetCameraComponent())
	{
		DeathCamera->GetCameraComponent()->bConstrainAspectRatio = false;
		DeathCamera->GetCameraComponent()->FieldOfView = 90.0f;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		if (!PlayerController || !PlayerController->IsLocalController())
		{
			continue;
		}

		if (DeathCamera)
		{
			PlayerController->SetViewTarget(DeathCamera);
		}

		if (DeathWidgetClass)
		{
			if (UUserWidget* DeathWidget = CreateWidget<UUserWidget>(PlayerController, DeathWidgetClass))
			{
				DeathWidget->AddToViewport();
				UE_LOG(LogTemp, Log, TEXT("[RunnerDeathSequence] DeathWidget added"));
			}
		}

		PlayerController->bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		PlayerController->SetInputMode(InputMode);
	}

	if (ASharedRailCharacter* SharedCharacter = Cast<ASharedRailCharacter>(UGameplayStatics::GetActorOfClass(this, ASharedRailCharacter::StaticClass())))
	{
		HideSharedCharacter(SharedCharacter);
	}
}

void ARunnerDeathSequence::RequestDeathAudio(const FVector& CageLocation)
{
	UE_LOG(LogTemp, Log, TEXT("[RunnerDeathSequence] Request Death BGM"));
	if (ACrazyRooAudioNetManager* AudioNetManager = ACrazyRooAudioNetManager::FindAudioNetManager(GetWorld()))
	{
		AudioNetManager->RequestPlayBGM(ECrazyRooBGMType::Death);
	}

	if (CageCloseSFXDelay <= 0.0f)
	{
		UE_LOG(LogTemp, Log, TEXT("[RunnerDeathSequence] Request CageClose SFX"));
		if (ACrazyRooAudioNetManager* AudioNetManager = ACrazyRooAudioNetManager::FindAudioNetManager(GetWorld()))
		{
			AudioNetManager->RequestPlaySFXAtLocation(ECrazyRooSFXType::CageClose, CageLocation);
		}
		return;
	}

	FTimerDelegate CageCloseDelegate;
	CageCloseDelegate.BindWeakLambda(this, [this, CageLocation]()
	{
		UE_LOG(LogTemp, Log, TEXT("[RunnerDeathSequence] Request CageClose SFX"));
		if (ACrazyRooAudioNetManager* AudioNetManager = ACrazyRooAudioNetManager::FindAudioNetManager(GetWorld()))
		{
			AudioNetManager->RequestPlaySFXAtLocation(ECrazyRooSFXType::CageClose, CageLocation);
		}
	});

	GetWorldTimerManager().SetTimer(CageCloseSFXTimerHandle, CageCloseDelegate, CageCloseSFXDelay, false);
}

ARunnerDeathSequence* ARunnerDeathSequence::FindDeathSequence(UWorld* World)
{
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunnerDeathSequence] FindDeathSequence failed: World is null"));
		return nullptr;
	}

	for (TActorIterator<ARunnerDeathSequence> It(World); It; ++It)
	{
		return *It;
	}

	UE_LOG(LogTemp, Warning, TEXT("[RunnerDeathSequence] FindDeathSequence failed: sequence not found"));
	return nullptr;
}

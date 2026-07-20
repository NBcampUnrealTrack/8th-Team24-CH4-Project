// Copyright Epic Games, Inc. All Rights Reserved.

#include "Stage/RunnerEndingBox.h"

#include "Audio/CrazyRooAudioNetManager.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraActor.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Enemies/FlyingEnemy.h"
#include "Enemies/FlyingEnemySpawner.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Manager/SharedCharacterManager.h"
#include "Camera/CameraComponent.h"
#include "Player/SharedRailCharacter.h"
#include "Manager/CrazyRooLocalPlayerSubsystem.h"

ARunnerEndingBox::ARunnerEndingBox()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	EndingBox = CreateDefaultSubobject<UBoxComponent>(TEXT("EndingBox"));
	EndingBox->SetupAttachment(SceneRoot);
	EndingBox->SetBoxExtent(FVector(200.0f, 450.0f, 200.0f));
	EndingBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	EndingBox->SetCollisionObjectType(ECC_WorldDynamic);
	EndingBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	EndingBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	EndingBox->SetGenerateOverlapEvents(true);

	DanceSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("DanceSpawnPoint"));
	DanceSpawnPoint->SetupAttachment(SceneRoot);

	ClearCameraPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ClearCameraPoint"));
	ClearCameraPoint->SetupAttachment(SceneRoot);
}

void ARunnerEndingBox::BeginPlay()
{
	Super::BeginPlay();

	if (EndingBox)
	{
		EndingBox->OnComponentBeginOverlap.AddDynamic(this, &ARunnerEndingBox::HandleEndingBoxBeginOverlap);
	}
}

void ARunnerEndingBox::HandleEndingBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !IsValidEndingTarget(OtherActor))
	{
		return;
	}

	if (bTriggerOnlyOnce && bHasTriggered)
	{
		return;
	}

	bHasTriggered = true;

	ASharedRailCharacter* SharedCharacter = ResolveSharedRailCharacter(OtherActor);
	UE_LOG(LogTemp, Log, TEXT("[RunnerEndingBox] Request StageClear BGM"));
	if (ACrazyRooAudioNetManager* AudioNetManager = ACrazyRooAudioNetManager::FindAudioNetManager(GetWorld()))
	{
		AudioNetManager->RequestPlayBGM(ECrazyRooBGMType::StageClear);
	}

	StopRunnerSystems(SharedCharacter);
	HideSharedCharacter(SharedCharacter);

	const FTransform DanceSpawnTransform = DanceSpawnPoint ? DanceSpawnPoint->GetComponentTransform() : GetActorTransform();
	const FTransform ClearCameraTransform = ClearCameraPoint ? ClearCameraPoint->GetComponentTransform() : GetActorTransform();
	MulticastPlayClearSequence(DanceSpawnTransform, ClearCameraTransform);
}

void ARunnerEndingBox::MulticastPlayClearSequence_Implementation(FTransform DanceSpawnTransform, FTransform ClearCameraTransform)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	PlayLocalClearSequence(DanceSpawnTransform, ClearCameraTransform);
}

bool ARunnerEndingBox::IsValidEndingTarget(AActor* OtherActor) const
{
	return IsValid(OtherActor)
		&& (OtherActor->IsA<ASharedRailCharacter>() || OtherActor->ActorHasTag(TEXT("Player")));
}

ASharedRailCharacter* ARunnerEndingBox::ResolveSharedRailCharacter(AActor* OverlapActor) const
{
	if (ASharedRailCharacter* SharedCharacter = Cast<ASharedRailCharacter>(OverlapActor))
	{
		return SharedCharacter;
	}

	return Cast<ASharedRailCharacter>(UGameplayStatics::GetActorOfClass(this, ASharedRailCharacter::StaticClass()));
}

void ARunnerEndingBox::StopRunnerSystems(ASharedRailCharacter* SharedCharacter)
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

void ARunnerEndingBox::HideSharedCharacter(ASharedRailCharacter* SharedCharacter) const
{
	if (!IsValid(SharedCharacter))
	{
		return;
	}

	SharedCharacter->SetActorHiddenInGame(true);
	SharedCharacter->SetActorEnableCollision(false);
}

void ARunnerEndingBox::PlayLocalClearSequence(const FTransform& DanceSpawnTransform, const FTransform& ClearCameraTransform)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (DanceActorClass)
	{
		World->SpawnActor<AActor>(DanceActorClass, DanceSpawnTransform);
	}

	UClass* CameraClassToSpawn = ClearCameraClass ? ClearCameraClass.Get() : ACameraActor::StaticClass();
	ACameraActor* ClearCamera = World->SpawnActor<ACameraActor>(CameraClassToSpawn, ClearCameraTransform);

	if (ClearCamera && ClearCamera->GetCameraComponent())
	{
		ClearCamera->GetCameraComponent()->bConstrainAspectRatio = false;
		ClearCamera->GetCameraComponent()->FieldOfView = 90.0f;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		if (!PlayerController || !PlayerController->IsLocalController())
		{
			continue;
		}

		if (ClearCamera)
		{
			PlayerController->SetViewTarget(ClearCamera);
		}

		// [무송] 클리어 화면이 뜨는 동안 인게임 메인 UI(HUD)를 잠시 숨김.
		// 위젯을 파괴하지 않으므로, 재시작(맵 트래블) 시 새 UI가 뜨거나 SetMainUIVisible(true)로 복구 가능
		if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
		{
			if (UCrazyRooLocalPlayerSubsystem* UISubsystem = LocalPlayer->GetSubsystem<UCrazyRooLocalPlayerSubsystem>())
			{
				UISubsystem->SetMainUIVisible(false);

				UISubsystem->SetMouseCursorHidden(false);
			}
		}
		//

		if (ClearWidgetClass)
		{
			if (UUserWidget* ClearWidget = CreateWidget<UUserWidget>(PlayerController, ClearWidgetClass))
			{
				ClearWidget->AddToViewport();
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

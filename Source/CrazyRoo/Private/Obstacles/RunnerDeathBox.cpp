// Copyright Epic Games, Inc. All Rights Reserved.

#include "Obstacles/RunnerDeathBox.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Manager/SharedCharacterManager.h"
#include "Player/SharedRailCharacter.h"

ARunnerDeathBox::ARunnerDeathBox()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	DeathBox = CreateDefaultSubobject<UBoxComponent>(TEXT("DeathBox"));
	DeathBox->SetupAttachment(SceneRoot);
	DeathBox->SetBoxExtent(FVector(200.0f, 450.0f, 200.0f));
	DeathBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DeathBox->SetCollisionObjectType(ECC_WorldDynamic);
	DeathBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	DeathBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	DeathBox->SetGenerateOverlapEvents(true);
}

void ARunnerDeathBox::BeginPlay()
{
	Super::BeginPlay();

	if (DeathBox)
	{
		DeathBox->OnComponentBeginOverlap.AddDynamic(this, &ARunnerDeathBox::HandleDeathBoxBeginOverlap);
	}
}

void ARunnerDeathBox::HandleDeathBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority())
	{
		return;
	}

	if (bTriggerOnlyOnce && bHasTriggered)
	{
		return;
	}

	if (bDebugDeathBox)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunnerDeathBox] %s overlapped by %s"), *GetName(), *GetNameSafe(OtherActor));
	}

	ASharedRailCharacter* SharedCharacter = nullptr;
	if (!IsValidDeathTarget(OtherActor, SharedCharacter))
	{
		return;
	}

	bHasTriggered = true;
	TriggerDeath(SharedCharacter);
}

bool ARunnerDeathBox::IsValidDeathTarget(AActor* OtherActor, ASharedRailCharacter*& OutSharedCharacter) const
{
	OutSharedCharacter = Cast<ASharedRailCharacter>(OtherActor);
	return IsValid(OutSharedCharacter);
}

void ARunnerDeathBox::TriggerDeath(ASharedRailCharacter* SharedCharacter)
{
	if (!IsValid(SharedCharacter))
	{
		return;
	}

	if (bStopMovementOnTrigger)
	{
		if (ASharedCharacterManager* SharedCharacterManager = Cast<ASharedCharacterManager>(UGameplayStatics::GetActorOfClass(this, ASharedCharacterManager::StaticClass())))
		{
			SharedCharacterManager->StopMove();
		}
		else if (bDebugDeathBox)
		{
			UE_LOG(LogTemp, Warning, TEXT("[RunnerDeathBox] %s could not find SharedCharacterManager. Death damage will still be applied."), *GetName());
		}
	}

	SharedCharacter->ApplyPlayerDamage(DeathDamage, this, true);

	if (bDebugDeathBox)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunnerDeathBox] %s triggered death for %s with damage %.1f"), *GetName(), *GetNameSafe(SharedCharacter), DeathDamage);
	}
}

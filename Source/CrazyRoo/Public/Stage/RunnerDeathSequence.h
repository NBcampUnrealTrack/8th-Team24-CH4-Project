// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "RunnerDeathSequence.generated.h"

class ACameraActor;
class ASharedRailCharacter;
class USceneComponent;
class UUserWidget;

UCLASS(Blueprintable)
class CRAZYROO_API ARunnerDeathSequence : public AActor
{
	GENERATED_BODY()

public:
	ARunnerDeathSequence();

	UFUNCTION(BlueprintCallable, Category = "Death Sequence")
	void StartDeathSequence(ASharedRailCharacter* DeadCharacter);

	UFUNCTION(BlueprintCallable, Category = "Death Sequence", meta = (WorldContext = "World"))
	static ARunnerDeathSequence* FindDeathSequence(UWorld* World);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	// Pre-placed cage sequence reference point. Used for CageClose SFX location, not actor spawning.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Death Sequence", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> CageSpawnPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Death Sequence", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> DeathCameraPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death Sequence")
	TSubclassOf<ACameraActor> DeathCameraClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death Sequence|UI")
	TSubclassOf<UUserWidget> DeathWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death Sequence")
	bool bTriggerOnlyOnce = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Death Sequence")
	bool bHasTriggered = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death Sequence|Audio", meta = (ClampMin = "0.0", Units = "s"))
	float CageCloseSFXDelay = 0.0f;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayDeathSequence(FTransform CageSpawnTransform, FTransform DeathCameraTransform);

	void StopRunnerSystems(ASharedRailCharacter* DeadCharacter);
	void HideSharedCharacter(ASharedRailCharacter* DeadCharacter) const;
	void PlayLocalDeathSequence(const FTransform& CageSpawnTransform, const FTransform& DeathCameraTransform);
	void RequestDeathAudio(const FVector& CageLocation);

private:
	FTimerHandle CageCloseSFXTimerHandle;
};

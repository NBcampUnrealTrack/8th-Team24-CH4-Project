// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RunnerEndingBox.generated.h"

class ACameraActor;
class ASharedRailCharacter;
class UBoxComponent;
class UPrimitiveComponent;
class USceneComponent;
class UUserWidget;

UCLASS(Blueprintable)
class CRAZYROO_API ARunnerEndingBox : public AActor
{
	GENERATED_BODY()

public:
	ARunnerEndingBox();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* EndingBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ending", meta = (AllowPrivateAccess = "true"))
	USceneComponent* DanceSpawnPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ending", meta = (AllowPrivateAccess = "true"))
	USceneComponent* ClearCameraPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending")
	TSubclassOf<AActor> DanceActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending")
	TSubclassOf<ACameraActor> ClearCameraClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending|UI")
	TSubclassOf<UUserWidget> ClearWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending")
	bool bTriggerOnlyOnce = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ending")
	bool bHasTriggered = false;

	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleEndingBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayClearSequence(FTransform DanceSpawnTransform, FTransform ClearCameraTransform);

	bool IsValidEndingTarget(AActor* OtherActor) const;
	ASharedRailCharacter* ResolveSharedRailCharacter(AActor* OverlapActor) const;
	void StopRunnerSystems(ASharedRailCharacter* SharedCharacter);
	void HideSharedCharacter(ASharedRailCharacter* SharedCharacter) const;
	void PlayLocalClearSequence(const FTransform& DanceSpawnTransform, const FTransform& ClearCameraTransform);
};

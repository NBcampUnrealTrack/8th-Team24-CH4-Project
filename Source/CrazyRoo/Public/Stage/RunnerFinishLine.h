// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RunnerFinishLine.generated.h"

class UBoxComponent;
class UNiagaraSystem;
class UPrimitiveComponent;
class USceneComponent;
class USoundBase;

UCLASS(Blueprintable)
class CRAZYROO_API ARunnerFinishLine : public AActor
{
	GENERATED_BODY()

public:
	ARunnerFinishLine();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* FinishLineBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Finish Line", meta = (AllowPrivateAccess = "true"))
	USceneComponent* LeftFireworkPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Finish Line", meta = (AllowPrivateAccess = "true"))
	USceneComponent* RightFireworkPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Finish Line|Effects")
	TObjectPtr<UNiagaraSystem> FireworkEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Finish Line|Effects")
	FVector FireworkScale = FVector(1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Finish Line|Effects")
	TObjectPtr<USoundBase> FireworkSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Finish Line|Effects", meta = (ClampMin = "0.0"))
	float FireworkSoundVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Finish Line|Effects", meta = (ClampMin = "0.0"))
	float FireworkSoundPitch = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Finish Line")
	bool bTriggerOnlyOnce = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Finish Line")
	bool bHasTriggered = false;

	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleFinishLineBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayFireworks(FVector LeftLocation, FVector RightLocation);

	bool IsValidFinishTarget(AActor* OtherActor) const;
	void StopEnemySystems();
};

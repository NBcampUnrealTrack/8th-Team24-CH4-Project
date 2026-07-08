// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RunnerDeathBox.generated.h"

class ASharedCharacterManager;
class ASharedRailCharacter;
class UBoxComponent;
class USceneComponent;

UCLASS(Blueprintable)
class CRAZYROO_API ARunnerDeathBox : public AActor
{
	GENERATED_BODY()

public:
	ARunnerDeathBox();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* DeathBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner DeathBox")
	bool bTriggerOnlyOnce = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner DeathBox", meta = (ClampMin = "0.0"))
	float DeathDamage = 9999.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner DeathBox")
	bool bStopMovementOnTrigger = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner DeathBox|Debug")
	bool bDebugDeathBox = false;

	bool bHasTriggered = false;

	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleDeathBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	bool IsValidDeathTarget(AActor* OtherActor, ASharedRailCharacter*& OutSharedCharacter) const;
	void TriggerDeath(ASharedRailCharacter* SharedCharacter);
};

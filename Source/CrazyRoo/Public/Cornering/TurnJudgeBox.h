// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RunnerTypes.h"
#include "TurnJudgeBox.generated.h"

class UBoxComponent;
class USceneComponent;
class ATurnJudgeBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTurnJudgeActorSignature, ATurnJudgeBox*, TurnJudgeBox, AActor*, PlayerActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FTurnResolvedSignature, ATurnJudgeBox*, TurnJudgeBox, AActor*, PlayerActor, ERunnerTurnDirection, CornerDirection, ERunnerTurnInputDirection, InputDirection);

UCLASS(Blueprintable)
class CRAZYROO_API ATurnJudgeBox : public AActor
{
	GENERATED_BODY()

public:
	ATurnJudgeBox();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* JudgeBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turn Judge", meta = (AllowPrivateAccess = "true"))
	USceneComponent* TurnSnapPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn Judge")
	ERunnerTurnDirection CornerDirection = ERunnerTurnDirection::LeftTurn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn Judge")
	ERunnerTurnJudgeMode TurnJudgeMode = ERunnerTurnJudgeMode::ForcedTurn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn Judge", meta = (ClampMin = "0.0", Units = "s"))
	float MaxValidTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn Judge")
	bool bResolveOnlyOnce = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turn Judge")
	bool bHasResolvedTurn = false;

	TMap<TWeakObjectPtr<AActor>, float> ActivePlayers;

public:
	UPROPERTY(BlueprintAssignable, Category = "Turn Judge|Events")
	FTurnJudgeActorSignature OnPlayerEnteredJudgeBox;

	UPROPERTY(BlueprintAssignable, Category = "Turn Judge|Events")
	FTurnJudgeActorSignature OnPlayerExitedJudgeBox;

	UPROPERTY(BlueprintAssignable, Category = "Turn Judge|Events")
	FTurnResolvedSignature OnTurnSucceeded;

	UPROPERTY(BlueprintAssignable, Category = "Turn Judge|Events")
	FTurnResolvedSignature OnTurnFailed;

	UFUNCTION(BlueprintCallable, Category = "Turn Judge")
	bool TryResolveTurn(AActor* PlayerActor, ERunnerTurnInputDirection InputDirection);

	UFUNCTION(BlueprintPure, Category = "Turn Judge")
	bool IsPlayerInJudgeBox(AActor* PlayerActor) const;

	UFUNCTION(BlueprintPure, Category = "Turn Judge")
	bool IsInputValidForCorner(ERunnerTurnInputDirection InputDirection) const;

	UFUNCTION(BlueprintPure, Category = "Turn Judge")
	ERunnerTurnDirection GetCornerDirection() const { return CornerDirection; }

	UFUNCTION(BlueprintPure, Category = "Turn Judge")
	float GetRecommendedYawDelta() const;

	UFUNCTION(BlueprintPure, Category = "Turn Judge")
	float GetRecommendedYawDeltaForInput(ERunnerTurnInputDirection InputDirection) const;

	UFUNCTION(BlueprintPure, Category = "Turn Judge")
	FRotator GetRecommendedYawRotation() const;

	UFUNCTION(BlueprintPure, Category = "Turn Judge")
	FTransform GetTurnSnapTransform() const;

	UFUNCTION(BlueprintCallable, Category = "Turn Judge")
	void ResetTurnResolved();

	UFUNCTION(BlueprintPure, Category = "Turn Judge")
	bool IsPlayerActor(AActor* Actor) const;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleJudgeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleJudgeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};

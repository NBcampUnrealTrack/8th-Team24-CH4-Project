// Copyright Epic Games, Inc. All Rights Reserved.

#include "Cornering/TurnJudgeBox.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Pawn.h"

ATurnJudgeBox::ATurnJudgeBox()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	JudgeBox = CreateDefaultSubobject<UBoxComponent>(TEXT("JudgeBox"));
	JudgeBox->SetupAttachment(SceneRoot);
	JudgeBox->SetBoxExtent(FVector(200.0f, 450.0f, 160.0f));
	JudgeBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	JudgeBox->SetCollisionObjectType(ECC_WorldDynamic);
	JudgeBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	JudgeBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	JudgeBox->SetGenerateOverlapEvents(true);
}

void ATurnJudgeBox::BeginPlay()
{
	Super::BeginPlay();

	JudgeBox->OnComponentBeginOverlap.AddDynamic(this, &ATurnJudgeBox::HandleJudgeBeginOverlap);
	JudgeBox->OnComponentEndOverlap.AddDynamic(this, &ATurnJudgeBox::HandleJudgeEndOverlap);
}

bool ATurnJudgeBox::TryResolveTurn(AActor* PlayerActor, ERunnerTurnInputDirection InputDirection)
{
	if (!IsValid(PlayerActor) || !IsPlayerInJudgeBox(PlayerActor))
	{
		return false;
	}

	const float* EnterTime = ActivePlayers.Find(TWeakObjectPtr<AActor>(PlayerActor));
	if (EnterTime && MaxValidTime > 0.0f && GetWorld() && GetWorld()->GetTimeSeconds() - *EnterTime > MaxValidTime)
	{
		ActivePlayers.Remove(TWeakObjectPtr<AActor>(PlayerActor));
		OnTurnFailed.Broadcast(this, PlayerActor, CornerDirection, InputDirection);
		return false;
	}

	const bool bSuccess = IsInputValidForCorner(InputDirection);
	if (bSuccess)
	{
		OnTurnSucceeded.Broadcast(this, PlayerActor, CornerDirection, InputDirection);
	}
	else
	{
		OnTurnFailed.Broadcast(this, PlayerActor, CornerDirection, InputDirection);
	}

	return bSuccess;
}

bool ATurnJudgeBox::IsPlayerInJudgeBox(AActor* PlayerActor) const
{
	return IsValid(PlayerActor) && ActivePlayers.Contains(TWeakObjectPtr<AActor>(PlayerActor));
}

bool ATurnJudgeBox::IsInputValidForCorner(ERunnerTurnInputDirection InputDirection) const
{
	if (TurnJudgeMode == ERunnerTurnJudgeMode::BranchTurn)
	{
		return InputDirection == ERunnerTurnInputDirection::LeftInput || InputDirection == ERunnerTurnInputDirection::RightInput;
	}

	return (CornerDirection == ERunnerTurnDirection::LeftTurn && InputDirection == ERunnerTurnInputDirection::LeftInput)
		|| (CornerDirection == ERunnerTurnDirection::RightTurn && InputDirection == ERunnerTurnInputDirection::RightInput);
}

float ATurnJudgeBox::GetRecommendedYawDelta() const
{
	return CornerDirection == ERunnerTurnDirection::LeftTurn ? -90.0f : 90.0f;
}

float ATurnJudgeBox::GetRecommendedYawDeltaForInput(ERunnerTurnInputDirection InputDirection) const
{
	return InputDirection == ERunnerTurnInputDirection::LeftInput ? -90.0f : 90.0f;
}

FRotator ATurnJudgeBox::GetRecommendedYawRotation() const
{
	return FRotator(0.0f, GetRecommendedYawDelta(), 0.0f);
}

bool ATurnJudgeBox::IsPlayerActor(AActor* Actor) const
{
	if (!IsValid(Actor))
	{
		return false;
	}

	if (Actor->ActorHasTag(TEXT("Player")))
	{
		return true;
	}

	const APawn* Pawn = Cast<APawn>(Actor);
	return Pawn && Pawn->IsPlayerControlled();
}

void ATurnJudgeBox::HandleJudgeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!IsPlayerActor(OtherActor))
	{
		return;
	}

	ActivePlayers.Add(TWeakObjectPtr<AActor>(OtherActor), GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f);
	OnPlayerEnteredJudgeBox.Broadcast(this, OtherActor);
}

void ATurnJudgeBox::HandleJudgeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (ActivePlayers.Remove(TWeakObjectPtr<AActor>(OtherActor)) > 0)
	{
		OnPlayerExitedJudgeBox.Broadcast(this, OtherActor);
	}
}

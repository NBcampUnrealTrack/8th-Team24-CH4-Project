// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "CrazyRooGameState.generated.h"

/**
 *
 */
UCLASS()
class CRAZYROO_API ACrazyRooGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
};

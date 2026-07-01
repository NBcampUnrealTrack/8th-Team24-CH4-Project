// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "CrazyRooGameState.generated.h"

/**
 *
 */

enum class ECrazyRooStageState : uint8;

// UI 가 바인딩할 델리게이트 — MatchState 가 바뀌면 알림
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCrazyRooMapChanged, FName, NewMapId);

UCLASS()
class CRAZYROO_API ACrazyRooGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ACrazyRooGameState();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 현재 맵 식별자 (데이터 테이블 조회 키, 클라에도 정확히 동기화됨)
	UFUNCTION(BlueprintPure, Category = "CrazyRoo")
	FName GetCurrentMapId() const;

	// [Server Only] GameMode가 트래블 완료 후 호출
	void SetCurrentMapId(FName NewMapId);

	UPROPERTY(BlueprintAssignable, Category = "CrazyRoo")
	FOnCrazyRooMapChanged OnCurrentMapChanged;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_CurrentMapId)
	FName CurrentMapId;

	UFUNCTION()
	void OnRep_CurrentMapId();

};

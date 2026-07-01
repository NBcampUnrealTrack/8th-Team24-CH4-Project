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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCrazyRooStageStateChanged, ECrazyRooStageState, NewState);

UCLASS()
class CRAZYROO_API ACrazyRooGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ACrazyRooGameState();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 현재 매치 상태 반환
	UFUNCTION(BlueprintPure, Category = "CrazyRoo")
	ECrazyRooStageState GetStageState() const;

	// [Server Only] 매치 상태를 변경하는 Setter 함수
	// GameMode에서 게임의 흐름을 제어할 때 호출
	void SetStageState(ECrazyRooStageState NewState);

	// 상태가 변경되었을 때 UI나 다른 블루프린트에서 이벤트를 받을 수 있도록 열어둔 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "CrazyRoo")
	FOnCrazyRooStageStateChanged OnStageStateChanged;

protected:
	// 현재 매치 상태를 저장하는 변수 (서버에서 관리하며 클라이언트로 복제됨)
	// 값이 변경되면 클라이언트 측에서는 OnRep_MatchState 함수가 자동으로 호출
	UPROPERTY(ReplicatedUsing = OnRep_StageState)
	ECrazyRooStageState StageState;

	// MatchState 변수가 서버로부터 클라이언트에 복제되었을 때(값이 바뀌었을 때) 호출되는 콜백 함수
	UFUNCTION()
	void OnRep_StageState();
};

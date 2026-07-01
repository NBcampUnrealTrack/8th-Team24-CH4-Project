// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Engine/World.h" //삭제예정
#include "CrazyRooLocalPlayerSubsystem.generated.h"

enum class ECrazyRooStageState : uint8;
class UUserWidget;
class ACrazyRooGameState;
/**
 *
 */
UCLASS()
class CRAZYROO_API UCrazyRooLocalPlayerSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** GameState 를 찾아 구독 (스스로도 부르고, PlayerController 안전망도 부름) */
	UFUNCTION(BlueprintCallable, Category = "CrazyRoo|UI")
	void BindToGameState();

protected:
	/** 새 맵 액터 준비됨 → 재구독 (World 이벤트 콜백) */
	void OnWorldReady(const UWorld::FActorsInitializedParams& Params);

	/** MatchState 바뀌면 호출 → 위젯 교체 */
	UFUNCTION()
	void HandleMatchStateChanged(ECrazyRooStageState NewState);

	/** 현재 맵의 UI 를 데이터 테이블에서 읽어 띄움 */
	void ShowStageUI();

	/** 현재 떠있는 위젯 */
	UPROPERTY()
	TObjectPtr<UUserWidget> CurrentWidget;

	/** 구독 중인 GameState (재구독 판단) */
	UPROPERTY()
	TWeakObjectPtr<ACrazyRooGameState> BoundGameState;
};

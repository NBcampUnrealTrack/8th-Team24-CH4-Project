// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Engine/World.h" //삭제예정
#include "CrazyRooLocalPlayerSubsystem.generated.h"

enum class ECrazyRooStageState : uint8;
class UUserWidget;
class ACrazyRooGameState;
//class UWorld;
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

	/**
	 * 입력모드를 명시적으로 전환한다.
	 * bShowUI = true  → 커서 보임, UI 조작 가능 (메뉴/설정창 등을 열 때)
	 * bShowUI = false → 커서 숨김, 게임 조작 전용 (실제 플레이 중)
	 * ESC로 설정창 열기, "게임 시작" 버튼 등 상황에 맞게 블루프린트/C++에서 자유롭게 호출.
	 */
	UFUNCTION(BlueprintCallable, Category = "CrazyRoo|UI")
	void SetUIInputMode(bool bShowUI);

protected:
	/** 새 월드 준비됨 → GameStateSetEvent 구독 시작 + 이미 있으면 즉시 바인딩 */
	void OnWorldReady(const UWorld::FActorsInitializedParams& Params);

	/** GameState가 실제로 세팅되는 정확한 순간 호출 (재시도 불필요한 직접 신호) */
	void OnGameStateSet(AGameStateBase* NewGameState);

	void BindToGameState(ACrazyRooGameState* GS);

	UFUNCTION()
	void HandleCurrentMapChanged(FName NewMapId);

	/** 지정 맵의 메인 UI를 데이터 테이블에서 읽어 띄움 (입력모드는 별도 SetUIInputMode로 관리) */
	void ShowStageUI(FName MapId);

	UPROPERTY()
	TObjectPtr<UUserWidget> CurrentWidget;

	UPROPERTY()
	TWeakObjectPtr<ACrazyRooGameState> BoundGameState;

	TWeakObjectPtr<UWorld> BoundWorld;
};

// CrazyRooCountdownWidget.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CrazyRooCountdownWidget.generated.h"

class UTextBlock;
class ACrazyRooGameState;

/**
 * 카운트다운 숫자를 화면에 표시하는 위젯입니다.
 *
 * GameState의 OnCountdownChanged 이벤트를 구독하고
 * 서버에서 복제된 카운트다운 값을 TextBlock에 표시합니다.
 */
UCLASS()
class CRAZYROO_API UCrazyRooCountdownWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// 위젯이 생성되어 화면에 준비될 때 호출됩니다.
	virtual void NativeConstruct() override;

	// 위젯이 제거될 때 호출됩니다.
	virtual void NativeDestruct() override;

	/**
	 * WBP_Countdown에 있는 TextBlock과 연결됩니다.
	 *
	 * 블루프린트의 TextBlock 이름도 반드시
	 * CountdownText여야 합니다.
	 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CountdownText;

	// 현재 이벤트를 구독하고 있는 GameState입니다.
	UPROPERTY()
	TObjectPtr<ACrazyRooGameState> CachedGameState;

	// GameState가 아직 준비되지 않았을 경우 재시도하기 위한 타이머
	FTimerHandle BindGameStateTimerHandle;

	/**
	 * GameState를 찾아 이벤트에 연결합니다.
	 */
	void BindToGameState();

	/**
	 * GameState 이벤트 연결을 해제합니다.
	 */
	void UnbindFromGameState();

	/**
	 * Countdown 값이 변경될 때 호출됩니다.
	 *
	 * AddDynamic에 연결하려면 반드시 UFUNCTION이어야 하며,
	 * 델리게이트와 매개변수 형식이 정확히 같아야 합니다.
	 */
	UFUNCTION()
	void HandleCountdownChanged(int32 NewCountdown);

	/**
	 * 카운트다운 숫자를 TextBlock에 표시합니다.
	 */
	void UpdateCountdownText(int32 NewCountdown);

	// GO!를 일정 시간 보여준 후 숨기기 위한 타이머
	FTimerHandle HideCountdownTimerHandle;

	// GO!를 숨기는 함수
	UFUNCTION()
	void HideCountdown();

	/**
	 * 카운트다운이 끝나고 GO!를 표시할 때 호출됩니다.
	 * 실제 애니메이션 재생은 자식 블루프린트에서 구현합니다.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Countdown")
	void PlayGoAnimation();

	/**
	 * 서희 추가
	 * 5, 4, 3, 2, 1 숫자가 갱신될 때마다 호출되는 애니메이션입니다.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Countdown")
	void PlayCountAnimation(int32 NewCountdown);
};

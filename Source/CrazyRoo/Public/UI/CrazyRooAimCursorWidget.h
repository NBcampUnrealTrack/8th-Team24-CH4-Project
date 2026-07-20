#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/CrazyRooTypes.h"
#include "CrazyRooAimCursorWidget.generated.h"

class UImage;
class ACrazyRooPlayerState;

/**
 * 슈터의 조준점과 다른 플레이어용 공유 조준점을 표시하는 위젯입니다.
 *
 * 네트워크 통신은 담당하지 않습니다.
 * - 슈터: 로컬 마우스 위치를 직접 따라갑니다.
 * - 비슈터: 서버에서 전달받은 월드 조준 위치를 화면 좌표로 변환합니다.
 */
UCLASS(Abstract, Blueprintable)
class CRAZYROO_API UCrazyRooAimCursorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * 현재 로컬 플레이어 역할에 맞춰 조준점 모드를 설정합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "CrazyRoo|Aim Cursor")
	void ApplyLocalRole(ECrazyRooPlayerRole LocalRole);

	/**
	 * GameState에서 전달받은 공유 조준 위치를 표시합니다.
	 * 슈터 본인은 로컬 마우스를 사용하므로 이 값을 무시합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "CrazyRoo|Aim Cursor")
	void UpdateSharedAimWorldLocation(
		const FVector& WorldLocation,
		bool bActive
	);

	/**
	 * 조준점 이미지를 표시하거나 숨깁니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "CrazyRoo|Aim Cursor")
	void SetAimVisible(bool bVisible);

protected:
	virtual void NativeConstruct() override;

	virtual void NativeTick(
		const FGeometry& MyGeometry,
		float InDeltaTime
	) override;

	virtual void NativeDestruct() override;

	/**
	 * 자식 WBP에 반드시 같은 이름의 Image가 있어야 합니다.
	 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> IMG_AimCursor;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aim Cursor|Style")
	float ShooterOpacity = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aim Cursor|Style")
	float ObserverOpacity = 0.25f;

	/**[무송]
     * 슈터가 아닌 플레이어 화면에서 공유 조준점을 목표 위치로 따라가게 하는 보간 속도.
     * 값이 클수록 빠르고 딱딱하게, 작을수록 부드럽지만 늦게 따라갑니다.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aim Cursor|Style")
    float AimInterpSpeed = 15.0f;


private:
	/** 현재 이 화면의 플레이어가 슈터인지 여부 */
	bool bIsLocalShooter = false;

	/** [무송] 서버에서 받은 최신 목표 조준 위치 (보간의 목표값) */
	FVector TargetAimWorldLocation = FVector::ZeroVector;

	/** [무송] 현재 화면에 표시 중인 보간된 조준 위치 */
	FVector CurrentAimWorldLocation = FVector::ZeroVector;

	/** [무송] 첫 데이터 수신 여부. 첫 수신 시에는 보간 없이 즉시 스냅합니다. */
	bool bHasTargetAim = false;

	/** [무송] 역할 변경 구독 대상 PlayerState (해제용) */
	TWeakObjectPtr<ACrazyRooPlayerState> BoundPlayerState;

	/** [무송] PlayerState의 로비 상태(역할 포함) 변경 시 호출 */
	UFUNCTION()
	void HandleOwnerStateChanged();

	/** [무송] PlayerState가 준비되었을 때 호출되는 콜백 함수 */
	UFUNCTION()
	void HandlePlayerStateReady(ACrazyRooPlayerState* PS);


	/** 이미지의 정중앙을 지정된 화면 좌표에 배치합니다. */
	void SetAimScreenPosition(const FVector2D& ScreenPosition);
};

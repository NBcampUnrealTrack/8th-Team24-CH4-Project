// CrazyRooQTEWidget.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/CrazyRooTypes.h"
#include "CrazyRooQTEWidget.generated.h"

class AGroupQTEBreakableWall;
class UTextBlock;
class UWidget;

UCLASS()
class CRAZYROO_API UCrazyRooQTEWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="QTE")
	void SetQTEWall(AGroupQTEBreakableWall* InQTEWall);

	UFUNCTION(BlueprintPure, Category="QTE")
	AGroupQTEBreakableWall* GetQTEWall() const { return QTEWall; }

	UFUNCTION(BlueprintPure, Category="QTE")
	bool IsQTEActive() const;

	UFUNCTION(BlueprintPure, Category="QTE")
	FText GetRemainingTimeText() const;

	UFUNCTION(BlueprintPure, Category="QTE")
	FText GetTotalPressText() const;

	UFUNCTION(BlueprintPure, Category="QTE")
	FText GetStateText() const;

	UFUNCTION(BlueprintPure, Category="QTE")
	FText GetProgressStageText() const;

	UFUNCTION(BlueprintPure, Category="QTE")
	FText GetCountdownText() const;

	UFUNCTION(BlueprintPure, Category="QTE")
	FText GetGuideMessageText() const;

	UFUNCTION(BlueprintPure, Category="QTE")
	int32 GetPressCountForRole(ECrazyRooPlayerRole PlayerRole) const;

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

	// 남은 시간을 표시하는 TextBlock입니다.
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> RemainingTimeText;

	// 팀 전체 입력 수를 표시하는 TextBlock입니다.
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TotalPressText;

	// QTE 상태를 표시하는 선택 TextBlock입니다.
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> StateText;

	// 벽 파괴 진행 단계를 표시하는 선택 TextBlock입니다.
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ProgressStageText;

	// 준비 시간 동안 크게 보여줄 카운트다운 TextBlock입니다.
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> CountdownText;

	// 준비 시간 동안 카운트다운 아래에 보여줄 안내 문구 TextBlock입니다.
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> GuideMessageText;

	// 역할별 개인 입력 수를 표시하는 TextBlock입니다.
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> LeftPressText;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> RightPressText;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> AcrobatPressText;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ShooterPressText;

	UFUNCTION(BlueprintCallable, Category="QTE")
	void RefreshQTEText();

	FText GetRolePressText(ECrazyRooPlayerRole PlayerRole) const;

	// 진행 중 QTE의 숫자, 배경, 개인 입력 슬롯 전체를 담는 루트
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UWidget> MainQTERoot;

	// 준비 카운트다운과 안내 문구 전체를 담는 루트
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UWidget> PreparationRoot;

private:
	UPROPERTY()
	TObjectPtr<AGroupQTEBreakableWall> QTEWall;
};

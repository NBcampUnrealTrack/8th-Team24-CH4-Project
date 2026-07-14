#include "UI/CrazyRooQTEWidget.h"

#include "Components/TextBlock.h"
#include "Obstacles/GroupQTEBreakableWall.h"

void UCrazyRooQTEWidget::SetQTEWall(AGroupQTEBreakableWall* InQTEWall)
{
	QTEWall = InQTEWall;
	RefreshQTEText();
}

bool UCrazyRooQTEWidget::IsQTEActive() const
{
	return QTEWall && QTEWall->IsQTEActive();
}

FText UCrazyRooQTEWidget::GetRemainingTimeText() const
{
	if (!QTEWall)
	{
		return FText::FromString(TEXT("00:00"));
	}

	return QTEWall->GetQTERemainingTimeText();
}

FText UCrazyRooQTEWidget::GetTotalPressText() const
{
	if (!QTEWall)
	{
		return FText::FromString(TEXT("0 / 0"));
	}

	return FText::FromString(FString::Printf(
		TEXT("%d / %d"),
		QTEWall->GetCurrentTotalPressCount(),
		QTEWall->GetRequiredTotalPressCount()));
}

FText UCrazyRooQTEWidget::GetStateText() const
{
	if (!QTEWall)
	{
		return FText::FromString(TEXT("대기"));
	}

	switch (QTEWall->GetQTEState())
	{
	case EGroupQTEState::Preparing:
		return FText::FromString(TEXT("준비"));

	case EGroupQTEState::Active:
		return FText::FromString(TEXT("입력"));

	case EGroupQTEState::Success:
		return FText::FromString(TEXT("성공"));

	case EGroupQTEState::Failed:
		return FText::FromString(TEXT("실패"));

	case EGroupQTEState::Idle:
	default:
		return FText::FromString(TEXT("대기"));
	}
}

FText UCrazyRooQTEWidget::GetProgressStageText() const
{
	if (!QTEWall)
	{
		return FText::FromString(TEXT("0"));
	}

	return FText::AsNumber(QTEWall->GetCurrentProgressStage());
}

FText UCrazyRooQTEWidget::GetCountdownText() const
{
	if (!QTEWall || !QTEWall->IsQTEPreparing())
	{
		return FText::GetEmpty();
	}

	const int32 RemainingSeconds = FMath::Max(1, FMath::CeilToInt(QTEWall->GetQTERemainingTime()));
	return FText::AsNumber(RemainingSeconds);
}

FText UCrazyRooQTEWidget::GetGuideMessageText() const
{
	if (!QTEWall || !QTEWall->IsQTEPreparing())
	{
		return FText::GetEmpty();
	}

	return FText::FromString(TEXT("다 함께 G 키를 눌러 바위를 부수세요!"));
}

int32 UCrazyRooQTEWidget::GetPressCountForRole(ECrazyRooPlayerRole PlayerRole) const
{
	if (!QTEWall)
	{
		return 0;
	}

	return QTEWall->GetPressCountForRole(PlayerRole);
}

void UCrazyRooQTEWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RefreshQTEText();
}

void UCrazyRooQTEWidget::NativeDestruct()
{
	QTEWall = nullptr;

	Super::NativeDestruct();
}

void UCrazyRooQTEWidget::RefreshQTEText()
{
	const bool bShowPreparationGuide = QTEWall && QTEWall->IsQTEPreparing();
	const ESlateVisibility MainQTEVisibility = bShowPreparationGuide ? ESlateVisibility::Hidden : ESlateVisibility::Visible;
	const ESlateVisibility PreparationVisibility = bShowPreparationGuide ? ESlateVisibility::Visible : ESlateVisibility::Hidden;

	if (RemainingTimeText)
	{
		RemainingTimeText->SetVisibility(MainQTEVisibility);
		RemainingTimeText->SetText(GetRemainingTimeText());
	}

	if (TotalPressText)
	{
		TotalPressText->SetVisibility(MainQTEVisibility);
		TotalPressText->SetText(GetTotalPressText());
	}

	if (StateText)
	{
		StateText->SetVisibility(MainQTEVisibility);
		StateText->SetText(GetStateText());
	}

	if (ProgressStageText)
	{
		ProgressStageText->SetVisibility(MainQTEVisibility);
		ProgressStageText->SetText(GetProgressStageText());
	}

	if (CountdownText)
	{
		CountdownText->SetVisibility(PreparationVisibility);
		CountdownText->SetText(GetCountdownText());
	}

	if (GuideMessageText)
	{
		GuideMessageText->SetVisibility(PreparationVisibility);
		GuideMessageText->SetText(GetGuideMessageText());
	}

	if (LeftPressText)
	{
		LeftPressText->SetVisibility(MainQTEVisibility);
		LeftPressText->SetText(GetRolePressText(ECrazyRooPlayerRole::P_Left));
	}

	if (RightPressText)
	{
		RightPressText->SetVisibility(MainQTEVisibility);
		RightPressText->SetText(GetRolePressText(ECrazyRooPlayerRole::P_Right));
	}

	if (AcrobatPressText)
	{
		AcrobatPressText->SetVisibility(MainQTEVisibility);
		AcrobatPressText->SetText(GetRolePressText(ECrazyRooPlayerRole::P_Acrobat));
	}

	if (ShooterPressText)
	{
		ShooterPressText->SetVisibility(MainQTEVisibility);
		ShooterPressText->SetText(GetRolePressText(ECrazyRooPlayerRole::P_Shooter));
	}
}

FText UCrazyRooQTEWidget::GetRolePressText(ECrazyRooPlayerRole PlayerRole) const
{
	return FText::AsNumber(GetPressCountForRole(PlayerRole));
}

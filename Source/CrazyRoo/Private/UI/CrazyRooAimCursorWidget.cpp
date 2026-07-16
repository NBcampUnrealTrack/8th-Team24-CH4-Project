#include "UI/CrazyRooAimCursorWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Player/CrazyRooPlayerController.h"

void UCrazyRooAimCursorWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!IMG_AimCursor)
	{
		return;
	}

	/*
	 * Alignment를 0.5, 0.5로 설정합니다.
	 */
	if (UCanvasPanelSlot* CursorSlot =
		UWidgetLayoutLibrary::SlotAsCanvasSlot(IMG_AimCursor))
	{
		CursorSlot->SetAnchors(FAnchors(0.0f, 0.0f));
		CursorSlot->SetAlignment(FVector2D(0.5f, 0.5f));

		// 디자이너에서 크기가 0으로 저장되는 문제를 방지합니다.
		CursorSlot->SetSize(FVector2D(96.0f, 96.0f));
	}
	else
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[AimCursor] IMG_AimCursor가 Canvas Panel의 직접 자식이 아닙니다.")
		);
	}

	// 역할과 위치를 확인하기 전에는 숨겨둡니다.
	SetAimVisible(false);

	// 조준점은 클릭이나 마우스 이벤트를 막지 않습니다.
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UCrazyRooAimCursorWidget::ApplyLocalRole(
	ECrazyRooPlayerRole LocalRole)
{
	if (!IMG_AimCursor)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[AimCursor] IMG_AimCursor 바인딩 실패")
		);
		return;
	}

	bIsLocalShooter =
		LocalRole == ECrazyRooPlayerRole::P_Shooter;

	if (bIsLocalShooter)
	{
		// 슈터는 선명한 조준점을 사용합니다.
		IMG_AimCursor->SetRenderOpacity(ShooterOpacity);
		SetAimVisible(true);
	}
	else
	{
		// 다른 플레이어는 같은 조준점을 흐리게 사용합니다.
		IMG_AimCursor->SetRenderOpacity(ObserverOpacity);

		// 서버에서 첫 조준 위치가 도착하기 전까지 숨깁니다.
		SetAimVisible(false);
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[AimCursor] ApplyLocalRole Role=%d Shooter=%d"),
		static_cast<int32>(LocalRole),
		bIsLocalShooter ? 1 : 0
	);

	// 슈터는 기본 마우스 커서를 숨깁니다.
	if (ACrazyRooPlayerController* CrazyRooPC =
		Cast<ACrazyRooPlayerController>(GetOwningPlayer()))
	{
		CrazyRooPC->SetInGameAimCursorMode(bIsLocalShooter);
	}
}

void UCrazyRooAimCursorWidget::NativeTick(
	const FGeometry& MyGeometry,
	float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 비슈터 화면은 서버에서 받은 위치로만 움직입니다.
	if (!bIsLocalShooter)
	{
		return;
	}

	APlayerController* PlayerController = GetOwningPlayer();

	if (!PlayerController)
	{
		return;
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;

	if (!PlayerController->GetMousePosition(MouseX, MouseY))
	{
		return;
	}

	/*
	 * GetMousePosition은 실제 화면 픽셀 좌표를 반환합니다.
	 * UMG의 DPI 배율로 나눠 Canvas 좌표로 변환합니다.
	 */
	const float ViewportScale =
		UWidgetLayoutLibrary::GetViewportScale(this);

	const float SafeViewportScale =
		FMath::Max(ViewportScale, KINDA_SMALL_NUMBER);

	const FVector2D WidgetPosition(
		MouseX / SafeViewportScale,
		MouseY / SafeViewportScale
	);

	SetAimScreenPosition(WidgetPosition);
}

void UCrazyRooAimCursorWidget::UpdateSharedAimWorldLocation(
	const FVector& WorldLocation,
	bool bActive)
{
	// 슈터는 자신의 로컬 마우스 위치를 사용합니다.
	if (bIsLocalShooter)
	{
		return;
	}

	if (!bActive)
	{
		SetAimVisible(false);
		return;
	}

	APlayerController* PlayerController = GetOwningPlayer();

	if (!PlayerController)
	{
		SetAimVisible(false);
		return;
	}

	FVector2D WidgetPosition;

	const bool bProjected =
		UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
			PlayerController,
			WorldLocation,
			WidgetPosition,
			true
		);

	if (!bProjected)
	{
		SetAimVisible(false);
		return;
	}

	SetAimScreenPosition(WidgetPosition);
	SetAimVisible(true);
}

void UCrazyRooAimCursorWidget::SetAimScreenPosition(
	const FVector2D& ScreenPosition)
{
	if (!IMG_AimCursor)
	{
		return;
	}

	if (UCanvasPanelSlot* CursorSlot =
		UWidgetLayoutLibrary::SlotAsCanvasSlot(IMG_AimCursor))
	{
		CursorSlot->SetPosition(ScreenPosition);
	}
}

void UCrazyRooAimCursorWidget::SetAimVisible(bool bVisible)
{
	if (!IMG_AimCursor)
	{
		return;
	}

	IMG_AimCursor->SetVisibility(
		bVisible
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Hidden
	);
}

void UCrazyRooAimCursorWidget::NativeDestruct()
{
	/*
	 * 인게임 HUD가 사라질 때 일반 마우스 커서를 복구합니다.
	 * 로비나 메뉴로 돌아갔을 때 커서가 계속 숨겨지는 것을 방지합니다.
	 */
	if (ACrazyRooPlayerController* CrazyRooPC =
		Cast<ACrazyRooPlayerController>(GetOwningPlayer()))
	{
		CrazyRooPC->RestoreDefaultMouseCursor();
	}

	Super::NativeDestruct();
}

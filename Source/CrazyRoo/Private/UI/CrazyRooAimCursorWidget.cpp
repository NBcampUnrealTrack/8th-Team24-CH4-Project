#include "UI/CrazyRooAimCursorWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Player/CrazyRooPlayerController.h"
#include "Player/CrazyRooPlayerState.h"
#include "TimerManager.h"

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

	// 심리스 트래블 직후에는 PlayerController->PlayerState가 이벤트 기반 PlayerState 구독기반으로 확인하여 불러오기
	if (ACrazyRooPlayerController* PC = Cast<ACrazyRooPlayerController>(GetOwningPlayer()))
	{
		if (ACrazyRooPlayerState* CrazyRooPS = Cast<ACrazyRooPlayerState>(PC->PlayerState))
		{
			// 운 좋게 이미 PlayerState가 있다면 즉시 실행
			HandlePlayerStateReady(CrazyRooPS);
		}
		else
		{
			// 아직 안 왔다면 도착할 때까지 안전하게 대기(구독)
			PC->OnPlayerStateReady.AddDynamic(this, &UCrazyRooAimCursorWidget::HandlePlayerStateReady);
		}
	}

}

void UCrazyRooAimCursorWidget::HandleOwnerStateChanged()
{
	// OnRep_PlayerRole → OnLobbyStateChanged 경유로 여기 도착합니다.
	if (ACrazyRooPlayerState* PS = BoundPlayerState.Get())
	{
		ApplyLocalRole(PS->GetPlayerRole());
	}
}

void UCrazyRooAimCursorWidget::HandlePlayerStateReady(ACrazyRooPlayerState* PS)
{
	if (!PS)
	{
		return;
	}

	// 현재 역할을 즉시 적용
	ApplyLocalRole(PS->GetPlayerRole());

	// 이후 역할이 바뀌는 순간 자동 갱신되도록 구독
	if (BoundPlayerState.Get() != PS)
	{
		if (BoundPlayerState.IsValid())
		{
			BoundPlayerState->OnLobbyStateChanged.RemoveDynamic(
				this,
				&UCrazyRooAimCursorWidget::HandleOwnerStateChanged
			);
		}

		PS->OnLobbyStateChanged.AddDynamic(
			this,
			&UCrazyRooAimCursorWidget::HandleOwnerStateChanged
		);
		BoundPlayerState = PS;
	}

	// 1회성 초기화이므로 구독 취소 (메모리 관리)
	if (ACrazyRooPlayerController* PC = Cast<ACrazyRooPlayerController>(GetOwningPlayer()))
	{
		PC->OnPlayerStateReady.RemoveDynamic(this, &UCrazyRooAimCursorWidget::HandlePlayerStateReady);
	}
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
		if (!bHasTargetAim)
		{
			SetAimVisible(false);
		}
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[AimCursor] ApplyLocalRole Role=%d Shooter=%d"),
		static_cast<int32>(LocalRole),
		bIsLocalShooter ? 1 : 0
	);

	// 슈터는 기본 마우스 커서를 숨깁니다.
	// [무송] 중복 로직이라서 주석처리 LocalSubSystem에 있음
	//if (ACrazyRooPlayerController* CrazyRooPC =
	//	Cast<ACrazyRooPlayerController>(GetOwningPlayer()))
	//{
	//	CrazyRooPC->SetInGameAimCursorMode(bIsLocalShooter);
	//}
}

void UCrazyRooAimCursorWidget::NativeTick(
	const FGeometry& MyGeometry,
	float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	APlayerController* PlayerController = GetOwningPlayer();

	if (!PlayerController)
	{
		return;
	}

	// ------------------------------------------------------------
	// 비슈터: 서버에서 받은 목표 위치를 향해 매 프레임 보간
	// ------------------------------------------------------------
	if (!bIsLocalShooter)
	{
		if (!bHasTargetAim)
		{
			return;
		}

		/*
		 * 20Hz로 도착하는 이산적인 위치를 그대로 찍으면 끊겨 보입니다.
		 * VInterpTo로 부드럽게 목표를 따라가고,
		 * 매 프레임 다시 프로젝션하므로 카메라가 움직여도
		 * 조준점이 월드 위치에 자연스럽게 붙어 있습니다.
		 */
		CurrentAimWorldLocation = FMath::VInterpTo(
			CurrentAimWorldLocation,
			TargetAimWorldLocation,
			InDeltaTime,
			AimInterpSpeed
		);

		FVector2D WidgetPosition;

		const bool bProjected =
			UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
				PlayerController,
				CurrentAimWorldLocation,
				WidgetPosition,
				true
			);

		if (bProjected)
		{
			SetAimScreenPosition(WidgetPosition);
			SetAimVisible(true);
		}
		else
		{
			SetAimVisible(false);
		}
		return;
	}

	// ------------------------------------------------------------
	// 슈터: 로컬 마우스 위치를 직접 따라감
	// ------------------------------------------------------------

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
		bHasTargetAim = false;
		SetAimVisible(false);
		return;
	}

	// 첫 수신이면 보간 없이 즉시 스냅해 멀리서 날아오는 연출을 방지합니다.
	if (!bHasTargetAim)
	{
		CurrentAimWorldLocation = WorldLocation;
		bHasTargetAim = true;
	}

	//APlayerController* PlayerController = GetOwningPlayer();
//
	//if (!PlayerController)
	//{
	//	SetAimVisible(false);
	//	return;
	//}
//
	//FVector2D WidgetPosition;
//
	//const bool bProjected =
	//	UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
	//		PlayerController,
	//		WorldLocation,
	//		WidgetPosition,
	//		true
	//	);
//
	//if (!bProjected)
	//{
	//	SetAimVisible(false);
	//	return;
	//}
//
	//SetAimScreenPosition(WidgetPosition);
	//SetAimVisible(true);

	// 여기서는 목표값만 갱신하고, 실제 이동은 NativeTick에서 처리합니다.
	TargetAimWorldLocation = WorldLocation;
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
	if (BoundPlayerState.IsValid())
	{
		BoundPlayerState->OnLobbyStateChanged.RemoveDynamic(
			this,
			&UCrazyRooAimCursorWidget::HandleOwnerStateChanged
		);
	}

	/*
	 * 인게임 HUD가 사라질 때 일반 마우스 커서를 복구합니다.
	 * 로비나 메뉴로 돌아갔을 때 커서가 계속 숨겨지는 것을 방지합니다.
	 */
	//[무송] 중복 로직이라서 주석처리 LocalSubSystem에 있음
	//if (ACrazyRooPlayerController* CrazyRooPC =
	//	Cast<ACrazyRooPlayerController>(GetOwningPlayer()))
	//{
	//	CrazyRooPC->RestoreDefaultMouseCursor();
	//}

	Super::NativeDestruct();
}

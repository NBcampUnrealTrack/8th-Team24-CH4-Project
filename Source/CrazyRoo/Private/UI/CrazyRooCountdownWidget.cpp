// CrazyRooCountdownWidget.cpp


#include "UI/CrazyRooCountdownWidget.h"

#include "System/CrazyRooGameState.h"

#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UCrazyRooCountdownWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 위젯 생성 시 GameState 이벤트에 연결을 시도합니다.
	BindToGameState();
}

void UCrazyRooCountdownWidget::NativeDestruct()
{
	// 위젯이 제거될 때 재시도 타이머를 정리합니다.
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(BindGameStateTimerHandle);
	}

	// 델리게이트 연결도 해제합니다.
	UnbindFromGameState();

	Super::NativeDestruct();
}

void UCrazyRooCountdownWidget::BindToGameState()
{
	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	ACrazyRooGameState* GameState =
		World->GetGameState<ACrazyRooGameState>();

	if (!GameState)
	{
		/*
		 * 클라이언트에서는 위젯이 GameState 복제보다 먼저
		 * 생성될 수 있습니다.
		 *
		 * 0.1초 뒤 다시 연결을 시도합니다.
		 */
		World->GetTimerManager().SetTimer(
			BindGameStateTimerHandle,
			this,
			&UCrazyRooCountdownWidget::BindToGameState,
			0.1f,
			false
		);

		return;
	}

	// 이미 같은 GameState에 연결되어 있으면 중복 바인딩하지 않습니다.
	if (CachedGameState == GameState)
	{
		return;
	}

	// 이전 GameState가 있다면 먼저 연결을 해제합니다.
	UnbindFromGameState();

	CachedGameState = GameState;

	/*
	 * GameState의 카운트다운 이벤트와
	 * 위젯의 처리 함수를 연결합니다.
	 *
	 * 앞으로 OnCountdownChanged.Broadcast(Value)가 실행될 때마다
	 * HandleCountdownChanged(Value)가 호출됩니다.
	 */
	CachedGameState->OnCountdownChanged.AddDynamic(this, &UCrazyRooCountdownWidget::HandleCountdownChanged);

	/*
	 * 위젯 생성 전에 이미 카운트다운이 시작되었을 수 있으므로
	 * 현재 GameState 값을 즉시 한 번 표시합니다.
	 */
	// [한길] 0712
	// ManualTest처럼 카운트다운을 시작하지 않는 모드에서는 초기 Countdown 값이 0입니다.
	// 바인딩 직후의 0은 GO!가 아니라 숨김으로 처리하고, 실제 카운트다운 종료 0은 delegate에서 표시합니다.
	const int32 CurrentCountdown = CachedGameState->GetCountdown();
	if (CurrentCountdown > 0)
	{
		UpdateCountdownText(CurrentCountdown);
	}
	else if (CountdownText)
	{
		CountdownText->SetVisibility(ESlateVisibility::Collapsed);
	}

	UE_LOG(LogTemp, Warning, TEXT("CountdownWidget bound to GameState"));
}

void UCrazyRooCountdownWidget::UnbindFromGameState()
{
	if (!CachedGameState)
	{
		return;
	}

	// 위젯이 제거된 뒤 이벤트가 호출되지 않도록 연결을 해제합니다.
	CachedGameState->OnCountdownChanged.RemoveDynamic(this, &UCrazyRooCountdownWidget::HandleCountdownChanged);

	CachedGameState = nullptr;
}

void UCrazyRooCountdownWidget::HandleCountdownChanged(int32 NewCountdown)
{
	UpdateCountdownText(NewCountdown);
}

void UCrazyRooCountdownWidget::UpdateCountdownText(
	int32 NewCountdown
)
{
	if (!CountdownText)
	{
		UE_LOG(LogTemp, Error, TEXT("CountdownText is NULL. Check BindWidget name."));

		return;
	}

	if (NewCountdown > 0)
	{
		// 이전 애니메이션 때문에 투명도가 0이 되었을 수도 있으므로 복원합니다.
		CountdownText->SetRenderOpacity(1.0f);

		// 5, 4, 3, 2, 1 표시
		CountdownText->SetText(FText::AsNumber(NewCountdown));

		CountdownText->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		/*
		 * 0이 되면 GO!를 표시합니다.
		 * 바로 숨기고 싶다면 Collapsed로 변경해도 됩니다.
		 */
		CountdownText->SetText(FText::FromString(TEXT("GO!")));

		CountdownText->SetVisibility(ESlateVisibility::Visible);

		// 자식 WBP_Countdown에서 Anim_GO를 재생하도록 요청합니다.
		PlayGoAnimation();

		// 1초 뒤 GO! 숨기기
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(HideCountdownTimerHandle);

			GetWorld()->GetTimerManager().SetTimer(
				HideCountdownTimerHandle,
				this,
				&UCrazyRooCountdownWidget::HideCountdown,
				1.0f,
				false
			);
		}
	}
}

void UCrazyRooCountdownWidget::HideCountdown()
{
	if (!CountdownText)
	{
		return;
	}

	CountdownText->SetVisibility(ESlateVisibility::Collapsed);
}

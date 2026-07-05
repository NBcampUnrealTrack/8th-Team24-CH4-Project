// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/CrazyRooLocalPlayerSubsystem.h"
#include "CrazyRoo.h"
#include "Types/CrazyRooTypes.h"
#include "System/CrazyRooGameState.h"
#include "System/CrazyRooGameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "Blueprint/UserWidget.h"


void UCrazyRooLocalPlayerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogCrazyRoo, Warning, TEXT("[UISubsystem] Initialize"));

	// 새 맵의 액터들이 준비되는 시점을 스스로 감지 → 재구독
	FWorldDelegates::OnWorldInitializedActors.AddUObject(this, &UCrazyRooLocalPlayerSubsystem::OnWorldReady);
}

void UCrazyRooLocalPlayerSubsystem::Deinitialize()
{
	// 서브시스템이 파괴될 때 등록했던 델리게이트들을 해제
	FWorldDelegates::OnWorldInitializedActors.RemoveAll(this);

	if (BoundWorld.IsValid())
	{
		BoundWorld->GameStateSetEvent.RemoveAll(this);
	}

	if (BoundGameState.IsValid())
	{
		BoundGameState->OnCurrentMapChanged.RemoveDynamic(this, &UCrazyRooLocalPlayerSubsystem::HandleCurrentMapChanged);
	}

	Super::Deinitialize();
}

// =====================================================================
// 월드 및 GameState 바인딩 (World & GameState Binding)
// =====================================================================

void UCrazyRooLocalPlayerSubsystem::OnWorldReady(const UWorld::FActorsInitializedParams& Params)
{
	// 에디터에서 플레이(PIE) 시 여러 개의 클라이언트 창을 띄울 경우, 다른 클라이언트의 월드 초기화 이벤트까지 감지될 수 있습니다.
	// 이를 방지하기 위해 이 서브시스템을 소유한 로컬 플레이어의 월드와 파라미터로 넘어온 월드가 일치하는지 검사합니다.
	ULocalPlayer* LP = GetLocalPlayer();
	if (!LP || Params.World != LP->GetWorld())
	{
		return;
	}

	UWorld* World = Params.World;

	// 이전에 구독했던 월드가 있다면 델리게이트를 해제하고 새 월드로 갱신합니다.
	if (BoundWorld.IsValid() && BoundWorld.Get() != World)
	{
		BoundWorld->GameStateSetEvent.RemoveAll(this);
	}
	BoundWorld = World;

	// 월드에 GameState가 세팅되는 순간을 감지하는 델리게이트를 등록합니다.
	World->GameStateSetEvent.AddUObject(this, &UCrazyRooLocalPlayerSubsystem::OnGameStateSet);

	// 델리게이트 등록 전에 이미 GameState가 스폰되어 있다면 즉시 바인딩을 시도합니다.
	if (ACrazyRooGameState* GS = World->GetGameState<ACrazyRooGameState>())
	{
		BindToGameState(GS);
	}
	else
	{
		UE_LOG(LogCrazyRoo, Warning, TEXT("[UISubsystem] GameState 아직 없음 → GameStateSetEvent 대기 중"));
	}
}

void UCrazyRooLocalPlayerSubsystem::OnGameStateSet(AGameStateBase* NewGameState)
{
	// 월드에서 GameState 생성이 완료되어 이벤트가 트리거되면 호출
	// 사용하는 커스텀 GameState로 캐스팅 후 바인딩합니다.
	if (ACrazyRooGameState* GS = Cast<ACrazyRooGameState>(NewGameState))
	{
		BindToGameState(GS);
	}
}

void UCrazyRooLocalPlayerSubsystem::BindToGameState(ACrazyRooGameState* GS)
{
	if (!GS || BoundGameState.Get() == GS)
	{
		return;
	}

	if (BoundGameState.IsValid())
	{
		BoundGameState->OnCurrentMapChanged.RemoveDynamic(this, &UCrazyRooLocalPlayerSubsystem::HandleCurrentMapChanged);
	}

	// 새로운 GameState의 맵 변경(OnCurrentMapChanged) 델리게이트를 구독
	// 이제 서버(GameState)에서 맵 ID가 바뀔 때마다 클라이언트의 이 서브시스템이 알림을 받게 됨
	GS->OnCurrentMapChanged.AddDynamic(this, &UCrazyRooLocalPlayerSubsystem::HandleCurrentMapChanged);
	BoundGameState = GS;

	UE_LOG(LogCrazyRoo, Warning, TEXT("[UISubsystem] GameState 구독 완료"));

	// 바인딩 직후(OnRep을 이미 놓쳤을 수 있으므로), 현재 GameState가 들고 있는 맵 ID를 기준으로 초기 UI를 즉시 세팅
	HandleCurrentMapChanged(GS->GetCurrentMapId());
}

void UCrazyRooLocalPlayerSubsystem::HandleCurrentMapChanged(FName NewMapId)
{
	UE_LOG(LogCrazyRoo, Warning, TEXT("[UISubsystem] 맵 변경 → %s"), *NewMapId.ToString());
	ShowStageUI(NewMapId);
}

// =====================================================================
// UI 관리 로직 (UI Management)
// =====================================================================

void UCrazyRooLocalPlayerSubsystem::ShowStageUI(FName MapId)
{
	ULocalPlayer* LP = GetLocalPlayer();
	APlayerController* PC = LP ? LP->GetPlayerController(LP->GetWorld()) : nullptr;
	UCrazyRooGameInstance* GI = LP ? Cast<UCrazyRooGameInstance>(LP->GetGameInstance()) : nullptr;
	if (!PC || !GI)
	{
		return;
	}

	// GameInstance를 통해 데이터 테이블에서 현재 맵(MapId)에 해당하는 정보를 가져옴
	const FCrazyRooStageInfo* Row = GI->FindStageRow(MapId);
	if (!Row)
	{
		UE_LOG(LogCrazyRoo, Warning, TEXT("[UISubsystem] 맵 '%s' 행 못찾음"), *MapId.ToString());
		return;
	}

	// 화면에 이미 띄워져 있는 기존 위젯이 있다면 제거
	if (CurrentWidget)
	{
		CurrentWidget->RemoveFromParent();
		CurrentWidget = nullptr;
	}

	// 데이터 테이블에 등록된 맵 전용 위젯 클래스를 동기적으로 메모리에 로드
	TSubclassOf<UUserWidget> UIClass = Row->StageUIClass.LoadSynchronous();
	if (UIClass)
	{
		CurrentWidget = CreateWidget<UUserWidget>(PC, UIClass);
		if (CurrentWidget)
		{
			CurrentWidget->AddToViewport();
			UE_LOG(LogCrazyRoo, Warning, TEXT("[UISubsystem] 메인 UI 띄움: %s"), *UIClass->GetName());
		}
	}
	else
	{
		UE_LOG(LogCrazyRoo, Warning, TEXT("[UISubsystem] 이 맵의 UI 없음 (StageUIClass 비어있음)"));
	}

	// ★ 변경: 맵 데이터의 MatchState로 자동 판단하던 로직 삭제.
	//   메인 UI가 방금 떴다는 건 곧 사용자가 상호작용해야 한다는 뜻이므로,
	//   일단 UI 입력모드로 시작한다. 이후 "게임 시작" 등은 SetUIInputMode(false)를
	//   게임 로직(블루프린트) 쪽에서 명시적으로 호출해서 전환한다.
	SetUIInputMode(true);
}


void UCrazyRooLocalPlayerSubsystem::SetUIInputMode(bool bShowUI)
{
	ULocalPlayer* LP = GetLocalPlayer();
	APlayerController* PC = LP ? LP->GetPlayerController(LP->GetWorld()) : nullptr;
	if (!PC)
	{
		return;
	}

	// bShowUI 플래그에 따라 입력 모드와 마우스 커서 표시 여부를 토글합니다.
	if (bShowUI)
	{
		PC->bShowMouseCursor = true;
		PC->SetInputMode(FInputModeGameAndUI());
	}
	else
	{
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}

	UE_LOG(LogCrazyRoo, Warning, TEXT("[UISubsystem] SetUIInputMode(%s)"), bShowUI ? TEXT("true") : TEXT("false"));
}

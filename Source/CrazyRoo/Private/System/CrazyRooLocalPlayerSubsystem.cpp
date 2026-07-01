// Fill out your copyright notice in the Description page of Project Settings.


#include "System/CrazyRooLocalPlayerSubsystem.h"
#include "CrazyRoo.h"
#include "CrazyRooTypes.h"
#include "System/CrazyRooGameState.h"
#include "System/CrazyRooGameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
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
	FWorldDelegates::OnWorldInitializedActors.RemoveAll(this);

	if (BoundGameState.IsValid())
	{
		BoundGameState->OnStageStateChanged.RemoveDynamic(this, &UCrazyRooLocalPlayerSubsystem::HandleMatchStateChanged);
	}
	Super::Deinitialize();
}

void UCrazyRooLocalPlayerSubsystem::OnWorldReady(const UWorld::FActorsInitializedParams& Params)
{
	// 내 로컬 플레이어의 World 와 같은 World 일 때만 반응 (PIE 다중 인스턴스 방어)
	ULocalPlayer* LP = GetLocalPlayer();
	if (LP && Params.World == LP->GetWorld())
	{
		BindToGameState();
	}
}

void UCrazyRooLocalPlayerSubsystem::BindToGameState()
{
	ULocalPlayer* LP = GetLocalPlayer();
	UWorld* World = LP ? LP->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	ACrazyRooGameState* GS = World->GetGameState<ACrazyRooGameState>();

	// ★ 여기에 방어 코드를 추가합니다! ★
	// GameState가 아직 스폰되지 않았거나, 캐스팅에 실패했다면 즉시 함수를 빠져나갑니다.
	if (!GS)
	{
		UE_LOG(LogCrazyRoo, Warning, TEXT("[UISubsystem] 아직 GameState가 스폰되지 않았습니다. (구독 보류)"));
		return;
	}

	// 이미 이 GameState 에 구독했으면 스킵 (중복 방지)
	if (BoundGameState.Get() == GS)
	{
		return;
	}

	// 이전 GameState 구독 해제 (트래블로 바뀐 경우)
	if (BoundGameState.IsValid())
	{
		BoundGameState->OnStageStateChanged.RemoveDynamic(this, &UCrazyRooLocalPlayerSubsystem::HandleMatchStateChanged);
	}

	// 새 GameState 구독 (이제 GS가 절대 nullptr이 아님을 보장하므로 안전합니다!)
	GS->OnStageStateChanged.AddDynamic(this, &UCrazyRooLocalPlayerSubsystem::HandleMatchStateChanged);
	BoundGameState = GS;

	UE_LOG(LogCrazyRoo, Warning, TEXT("[UISubsystem] GameState 구독 완료"));

	// 구독 시점의 현재 상태 즉시 반영 (OnRep 을 이미 놓쳤을 수 있으니)
	HandleMatchStateChanged(GS->GetStageState());
}

void UCrazyRooLocalPlayerSubsystem::HandleMatchStateChanged(ECrazyRooStageState NewState)
{
	UE_LOG(LogCrazyRoo, Warning, TEXT("[UISubsystem] MatchState 변경 → %s"), CrazyRooMatchStateToString(NewState));

	// 상태가 뭐든, 현재 맵의 UI 를 데이터 테이블에서 읽어 띄움
	ShowStageUI();

	// 입력 모드 (상태별)
	APlayerController* PC = GetLocalPlayer() ? GetLocalPlayer()->GetPlayerController(GetLocalPlayer()->GetWorld()) : nullptr;
	if (PC)
	{
		if (NewState == ECrazyRooStageState::InGame)
		{
			PC->bShowMouseCursor = false;
			PC->SetInputMode(FInputModeGameOnly());
		}
		else
		{
			PC->bShowMouseCursor = true;
			PC->SetInputMode(FInputModeGameAndUI());
		}
	}
}

void UCrazyRooLocalPlayerSubsystem::ShowStageUI()
{
	ULocalPlayer* LP = GetLocalPlayer();
	APlayerController* PC = LP ? LP->GetPlayerController(LP->GetWorld()) : nullptr;
	UCrazyRooGameInstance* GI = LP ? Cast<UCrazyRooGameInstance>(LP->GetGameInstance()) : nullptr;
	if (!PC || !GI)
	{
		return;
	}

	// 이전 위젯 제거
	if (CurrentWidget)
	{
		CurrentWidget->RemoveFromParent();
		CurrentWidget = nullptr;
	}

	// 데이터 테이블에서 현재 맵의 UI 클래스
	TSubclassOf<UUserWidget> UIClass = GI->GetCurrentStageUIClass();
	if (UIClass)
	{
		CurrentWidget = CreateWidget<UUserWidget>(PC, UIClass);
		if (CurrentWidget)
		{
			CurrentWidget->AddToViewport();
			UE_LOG(LogCrazyRoo, Warning, TEXT("[UISubsystem] 위젯 띄움: %s"), *UIClass->GetName());
		}
	}
	else
	{
		UE_LOG(LogCrazyRoo, Warning, TEXT("[UISubsystem] 이 맵의 UI 없음 (StageUIClass 비어있음)"));
	}
}

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

void UCrazyRooLocalPlayerSubsystem::OnWorldReady(const UWorld::FActorsInitializedParams& Params)
{
	ULocalPlayer* LP = GetLocalPlayer();
	if (!LP || Params.World != LP->GetWorld())
	{
		return;
	}

	UWorld* World = Params.World;

	if (BoundWorld.IsValid() && BoundWorld.Get() != World)
	{
		BoundWorld->GameStateSetEvent.RemoveAll(this);
	}
	BoundWorld = World;

	World->GameStateSetEvent.AddUObject(this, &UCrazyRooLocalPlayerSubsystem::OnGameStateSet);

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

	GS->OnCurrentMapChanged.AddDynamic(this, &UCrazyRooLocalPlayerSubsystem::HandleCurrentMapChanged);
	BoundGameState = GS;

	UE_LOG(LogCrazyRoo, Warning, TEXT("[UISubsystem] GameState 구독 완료"));

	HandleCurrentMapChanged(GS->GetCurrentMapId());
}

void UCrazyRooLocalPlayerSubsystem::HandleCurrentMapChanged(FName NewMapId)
{
	UE_LOG(LogCrazyRoo, Warning, TEXT("[UISubsystem] 맵 변경 → %s"), *NewMapId.ToString());
	ShowStageUI(NewMapId);
}

void UCrazyRooLocalPlayerSubsystem::ShowStageUI(FName MapId)
{
	ULocalPlayer* LP = GetLocalPlayer();
	APlayerController* PC = LP ? LP->GetPlayerController(LP->GetWorld()) : nullptr;
	UCrazyRooGameInstance* GI = LP ? Cast<UCrazyRooGameInstance>(LP->GetGameInstance()) : nullptr;
	if (!PC || !GI)
	{
		return;
	}

	const FCrazyRooStageInfo* Row = GI->FindStageRow(MapId);
	if (!Row)
	{
		UE_LOG(LogCrazyRoo, Warning, TEXT("[UISubsystem] 맵 '%s' 행 못찾음"), *MapId.ToString());
		return;
	}

	// 이전 메인 UI 제거
	if (CurrentWidget)
	{
		CurrentWidget->RemoveFromParent();
		CurrentWidget = nullptr;
	}

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

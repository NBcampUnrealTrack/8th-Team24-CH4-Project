// Copyright Epic Games, Inc. All Rights Reserved.


#include "CrazyRooPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "CrazyRoo.h"
#include "CrazyRooGameMode.h"
#include "CrazyRooTypes.h"
#include "Widgets/Input/SVirtualJoystick.h"

void ACrazyRooPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 서버엔 모든 PC, 각 클라엔 자기 것(Local=1) 하나만 존재
	const FString Msg = FString::Printf(
		TEXT("PlayerController BeginPlay (Local=%d)"),
		IsLocalController() ? 1 : 0);
	FCrazyRooDebug::Print(this, Msg, FColor::Magenta);


	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogCrazyRoo, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}

	// IsLocalPlayerController()는 이 컨트롤러가 내 컴퓨터에 연결된 것인지 묻는 함수입니다.
	// 서버에는 4명의 컨트롤러 다 있지만, UI는 내 화면에만 띄워야 하므로 검사합니다.

	// 역할 선택 위젯: 내 로컬 클라에서만 띄움. 데디 서버는 화면이 없으므로 제외됨.
	if (IsLocalController() && RoleSelectWidgetClass)
	{
		RoleSelectWidget = CreateWidget<UUserWidget>(this, RoleSelectWidgetClass);
		if (RoleSelectWidget)
		{
			RoleSelectWidget->AddToViewport();
			bShowMouseCursor = true;// 마우스 활성화
			SetInputMode(FInputModeGameAndUI());  // 마우스와 키보드 모두 작동하게 설정
			FCrazyRooDebug::Print(this, TEXT("역할 선택 위젯 표시"), FColor::Cyan);
		}
	}

}

void ACrazyRooPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

bool ACrazyRooPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

// UI의 버튼을 클릭하면 블루프린트에서 이 함수를 호출
void ACrazyRooPlayerController::RequestRole(ECrazyRooPlayerRole DesiredRole)
{
	// 즉시 서버 RPC 호출
	ServerRequestRole(DesiredRole);
}

void ACrazyRooPlayerController::RequestMap(FName MapId)
{
	ServerTravel(MapId);
}

void ACrazyRooPlayerController::ServerTravel_Implementation(FName MapId)
{
	// 서버에서 실행됨 → GameMode 에 트래블 위임
	if (ACrazyRooGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ACrazyRooGameMode>() : nullptr)
	{
		GM->ChangeMap(MapId);
	}
}


void ACrazyRooPlayerController::ServerRequestRole_Implementation(ECrazyRooPlayerRole DesiredRole)
{
	// 서버에서 실행됨
	if (ACrazyRooGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ACrazyRooGameMode>() : nullptr)
	{
		if (!GM->RequestRole(this, DesiredRole))
		{
			ClientRoleRejected(DesiredRole);   // 거부 → 요청한 클라에게만
		}
	}
}

void ACrazyRooPlayerController::ClientRoleRejected_Implementation(ECrazyRooPlayerRole DeniedRole)
{
	// 그 클라에서 실행됨 → UI 메시지
	const FString Msg = FString::Printf(TEXT("[거부] %s 는 이미 배정된 역할입니다"),
		CrazyRooRoleToString(DeniedRole));
	FCrazyRooDebug::Print(this, Msg, FColor::Red);

	OnRoleRejected.Broadcast(DeniedRole);
}

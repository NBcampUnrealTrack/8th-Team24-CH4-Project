// Copyright Epic Games, Inc. All Rights Reserved.


#include "CrazyRooPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "CrazyRoo.h"
#include "CrazyRooGameMode.h"
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

	// 역할 선택 위젯: 로컬 클라에서만 띄움 (데디 서버는 화면 없으니 자동 제외)
	if (IsLocalController() && RoleSelectWidgetClass)
	{
		RoleSelectWidget = CreateWidget<UUserWidget>(this, RoleSelectWidgetClass);
		if (RoleSelectWidget)
		{
			RoleSelectWidget->AddToViewport();
			bShowMouseCursor = true;
			SetInputMode(FInputModeGameAndUI());   // 버튼 클릭 + 키 둘 다
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

void ACrazyRooPlayerController::RequestRole(ECrazyRooPlayerRole DesiredRole)
{
	// 버튼은 서버에 요청만. 판정은 서버
	ServerRequestRole(DesiredRole);
}

void ACrazyRooPlayerController::ClientRoleRejected_Implementation(ECrazyRooPlayerRole DeniedRole)
{
	// 그 클라에서 실행됨 → UI 메시지
	const FString Msg = FString::Printf(TEXT("[거부] %s 는 이미 배정된 역할입니다"),
		CrazyRooRoleToString(DeniedRole));
	FCrazyRooDebug::Print(this, Msg, FColor::Red);

	OnRoleRejected.Broadcast(DeniedRole);
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

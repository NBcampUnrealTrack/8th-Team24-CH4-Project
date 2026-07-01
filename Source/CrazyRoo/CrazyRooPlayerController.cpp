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
#include "Components/InputComponent.h"


void ACrazyRooPlayerController::BeginPlay()
{
	Super::BeginPlay();


	// 서버엔 모든 PC, 각 클라엔 자기 것(Local=1) 하나만 존재
	const FString Msg = FString::Printf(
		TEXT("PlayerController BeginPlay (Local=%d)"),
		IsLocalController() ? 1 : 0);
	FCrazyRooDebug::Print(this, Msg, FColor::Magenta);//PlayerState 생성보다 먼저이기 때문에 Player?가 나오는게 당연하다


	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		}
		else
		{

			UE_LOG(LogCrazyRoo, Error, TEXT("Could not spawn mobile controls widget."));
		}
	}

}

void ACrazyRooPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	//테스트용 삭제 예정
	//if (InputComponent)
	//{
	//	// 테스트용: T 키 누르면 TestStage 로 트래블
	//	InputComponent->BindKey(EKeys::T, IE_Pressed, this, &ACrazyRooPlayerController::Key_TravelToStage);
	//}
	//

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

//테스트용 삭제예정
//void ACrazyRooPlayerController::Key_TravelToStage()
//{
//	FCrazyRooDebug::Print(this, TEXT("KEY T -> TestStage 트래블 요청"), FColor::Cyan);
//	RequestMap(TEXT("TestStage"));
//}
//

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

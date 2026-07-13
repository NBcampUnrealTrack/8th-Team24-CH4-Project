// Copyright Epic Games, Inc. All Rights Reserved.


#include "Player/CrazyRooPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "CrazyRoo.h"
#include "System/CrazyRooGameMode.h"
#include "System/CrazyRooGameState.h"
#include "Player/CrazyRooPlayerState.h"
#include "Types/CrazyRooTypes.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "Manager/SharedCharacterManager.h"
#include "Player/SharedRailCharacter.h"
#include "Input/RailInputConfig.h"
#include "Components/InputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "InputActionValue.h" // 테슽트용 앞 뒤 움직임.
#include "System/CrazyRooGameInstance.h"
#include "UI/CrazyRooCountdownWidget.h" // [한길] 0711 카운트다운 위젯


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

	//  게임 맵이라면 공유 캐릭터로 카메라 전환 (로비 맵에선 못 찾으니 자동 스킵)
	//if (IsLocalController())
	//{
	//	ASharedRailCharacter* SharedCharacter = Cast<ASharedRailCharacter>(
	//		UGameplayStatics::GetActorOfClass(GetWorld(), ASharedRailCharacter::StaticClass()));

	//	if (SharedCharacter)
	//	{
	//		SetViewTargetWithBlend(SharedCharacter, 0.0f);
	//		UE_LOG(LogCrazyRoo, Warning, TEXT("ViewTarget set to SharedCharacter"));
	//	}
	//}

	// [한길] 0711 카운트다운 UI
	if (!IsLocalController())
	{
		return;
	}
	// ---------------------------------------------------------
	// 카운트다운 UI 생성
	// ---------------------------------------------------------
	if (!CountdownWidgetClass)
	{
		UE_LOG(LogCrazyRoo, Error, TEXT("CountdownWidgetClass is NULL"));

		return;
	}

	CountdownWidget = CreateWidget<UCrazyRooCountdownWidget>(this, CountdownWidgetClass);

	if (!CountdownWidget)
	{
		UE_LOG(LogCrazyRoo, Error, TEXT("CountdownWidget creation failed"));

		return;
	}

	// 각 플레이어 자신의 화면에 위젯을 추가합니다.
	CountdownWidget->AddToPlayerScreen(10);

	UE_LOG(LogCrazyRoo, Warning, TEXT("CountdownWidget added to player screen"));

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
			// 공유 캐릭터 조작용 IMC (InputConfig가 세팅된 경우만)
			if (InputConfig && InputConfig->InputMappingContext)
			{
				Subsystem->AddMappingContext(InputConfig->InputMappingContext, 1);
			}
		}
		// ── 공유 캐릭터 조작 액션 바인딩 ──────────
		UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
		if (!EnhancedInput || !InputConfig)
		{
			return;
		}

		if (InputConfig->IA_Left)
		{
			EnhancedInput->BindAction(InputConfig->IA_Left, ETriggerEvent::Started, this, &ACrazyRooPlayerController::InputLeft);
		}
		if (InputConfig->IA_Right)
		{
			EnhancedInput->BindAction(InputConfig->IA_Right, ETriggerEvent::Started, this, &ACrazyRooPlayerController::InputRight);
		}
		if (InputConfig->IA_Jump)
		{
			EnhancedInput->BindAction(InputConfig->IA_Jump, ETriggerEvent::Started, this, &ACrazyRooPlayerController::InputJump);
		}
		if (InputConfig->IA_Slide)
		{
			EnhancedInput->BindAction(InputConfig->IA_Slide, ETriggerEvent::Started, this, &ACrazyRooPlayerController::InputSlide);
		}
		if (InputConfig->IA_Fire)
		{
			EnhancedInput->BindAction(InputConfig->IA_Fire, ETriggerEvent::Started, this, &ACrazyRooPlayerController::InputFire);
		}
		if (InputConfig->IA_CameraLeft)
		{
			EnhancedInput->BindAction(InputConfig->IA_CameraLeft, ETriggerEvent::Started, this, &ACrazyRooPlayerController::InputCameraLeft);
		}
		if (InputConfig->IA_CameraRight)
		{
			EnhancedInput->BindAction(InputConfig->IA_CameraRight, ETriggerEvent::Started, this, &ACrazyRooPlayerController::InputCameraRight);
		}
		// [한길] 0703 테스트용 앞뒤 움직임
		if (InputConfig->IA_Move)
		{
			EnhancedInput->BindAction(InputConfig->IA_Move, ETriggerEvent::Triggered, this, &ACrazyRooPlayerController::InputMove);
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

// [준비] 버튼 → 서버 RPC
void ACrazyRooPlayerController::RequestReady(bool bReady)
{
	ServerSetReady(bReady);
}

void ACrazyRooPlayerController::RequestMap(FName MapId)
{
	ServerTravel(MapId);
}

void ACrazyRooPlayerController::RequestUpdateCarousel(int32 NewIndex)
{
	Server_UpdateCarousel(NewIndex);
}

void ACrazyRooPlayerController::Client_SetupGameplayCamera_Implementation()
{
	// 데이터 테이블에서 게임 맵이라고 받아오면 공유 캐릭터로 카메라 전환
	ASharedRailCharacter* SharedCharacter = Cast<ASharedRailCharacter>(
		UGameplayStatics::GetActorOfClass(GetWorld(), ASharedRailCharacter::StaticClass()));

	if (SharedCharacter)
	{
		SetViewTargetWithBlend(SharedCharacter, 0.0f);
		UE_LOG(LogCrazyRoo, Warning, TEXT("[Client] ViewTarget set to SharedCharacter"));
	}
	else
	{
		UE_LOG(LogCrazyRoo, Error, TEXT("[Client] SharedCharacter를 찾을 수 없습니다!"));
	}
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
	//if (ACrazyRooGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ACrazyRooGameMode>() : nullptr)
	//{
	//	GM->ChangeMap(MapId);
	//}

	// 서버에서 실행됨
	const ACrazyRooPlayerState* PS = GetPlayerState<ACrazyRooPlayerState>();
	ACrazyRooGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ACrazyRooGameMode>() : nullptr;
	UCrazyRooGameInstance* GI = GetGameInstance<UCrazyRooGameInstance>();

	if (!PS || !GM || !GI)
	{
		return;
	}

	// 방장만 맵 이동 요청 가능 (모든 맵 공통 — 클라 UI를 우회한 치트성 호출도 여기서 차단)
	if (!PS->IsHost())
	{
		FCrazyRooDebug::Print(this, FString::Printf(
			TEXT("[SERVER] 플레이어-%d 트래블 요청 거부: 방장이 아님"), PS->GetJoinOrder()), FColor::Red);
		return;
	}

	// 로비에서 출발할 때만 시작 조건 검사
	if (GI->CurrentMapId == TEXT("TestMap")) //여기서 역할 선택 방 선택 이후로 넘어가는 맵(데이터 에셋 Row) 이름이 같아야 함
	{
		if (GM->IsDevSoloStart())
		{
			// [Dev] 테스트 규칙: 방장이 역할만 골랐으면 통과 (인원/준비 검사 생략)
			if (PS->GetPlayerRole() == ECrazyRooPlayerRole::None)
			{
				FCrazyRooDebug::Print(this, TEXT("[SERVER] 트래블 거부: 역할을 먼저 선택하세요"), FColor::Red);
				return;
			}
		}
		else if (!GM->AreAllPlayersReady())
		{
			// 정식 규칙: 최소 인원 + 전원 준비
			FCrazyRooDebug::Print(this, TEXT("[SERVER] 트래블 거부: 인원 부족 또는 전원 준비 안 됨"), FColor::Red);
			return;
		}
	}

	// 검증 통과 → GameMode 에 트래블 위임
	GM->ChangeMap(MapId);
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

// ── 공유 캐릭터 조작 (RailCoopPlayerController 흡수- 한길님 파트) ──

void ACrazyRooPlayerController::ServerSetReady_Implementation(bool bReady)
{
	if (ACrazyRooPlayerState* PS = GetPlayerState<ACrazyRooPlayerState>())
	{
		// 예외 처리: 캐릭터(역할)를 선택하지 않은 빈 상태(None)에서는 '준비 완료'를 누를 수 없도록 막습니다.
		if (bReady && PS->GetPlayerRole() == ECrazyRooPlayerRole::None)
		{
			return;
		}

		// 조건을 통과했다면 PlayerState의 레디 상태를 업데이트합니다.
		// (bIsReady 변수는 Replicated 처리되어 있으므로 알아서 모든 클라이언트 화면에 갱신됩니다)
		PS->SetIsReady(bReady);
	}
}

void ACrazyRooPlayerController::Server_UpdateCarousel_Implementation(int32 NewIndex)
{
	// 요청한 사람이 방장인지 확인 (아무나 맵을 바꾸면 안 되니까!)
	if (ACrazyRooPlayerState* PS = GetPlayerState<ACrazyRooPlayerState>())
	{
		if (PS->IsHost())
		{
			// 방장이 맞으면 GameState의 인덱스를 바꿈
			if (ACrazyRooGameState* GS = GetWorld()->GetGameState<ACrazyRooGameState>())
			{
				GS->SetCarouselIndex(NewIndex);
			}
		}
	}
}

void ACrazyRooPlayerController::InputLeft()
{
	Server_RequestMoveLeft();
}

void ACrazyRooPlayerController::InputRight()
{
	Server_RequestMoveRight();
}

void ACrazyRooPlayerController::InputJump()
{
	Server_RequestJump();
}

void ACrazyRooPlayerController::InputSlide()
{
	Server_RequestSlide();
}

void ACrazyRooPlayerController::InputFire()
{
	FVector AimOrigin;
	FVector AimDirection;

	if (!GetMouseAimRay(AimOrigin, AimDirection))
	{
		return;
	}

	Server_RequestFireAimRay(AimOrigin, AimDirection);
}

void ACrazyRooPlayerController::InputCameraLeft()
{
	Server_RequestRotateCameraLeft();
}

void ACrazyRooPlayerController::InputCameraRight()
{
	Server_RequestRotateCameraRight();
}

bool ACrazyRooPlayerController::GetMouseAimRay(FVector& OutOrigin, FVector& OutDirection) const
{
	if (!DeprojectMousePositionToWorld(OutOrigin, OutDirection))
	{
		return false;
	}

	OutDirection.Normalize();
	return true;
}

ASharedCharacterManager* ACrazyRooPlayerController::GetManagerOnServer() const
{
	ACrazyRooGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ACrazyRooGameMode>() : nullptr;
	return GM ? GM->GetSharedCharacterManager() : nullptr;
}

ECrazyRooPlayerRole ACrazyRooPlayerController::GetMyRoleOnServer() const
{
	// ★ 역할의 진실은 PlayerState 하나 (컨트롤러에 중복 저장 안 함)
	const ACrazyRooPlayerState* PS = GetPlayerState<ACrazyRooPlayerState>();
	return PS ? PS->GetPlayerRole() : ECrazyRooPlayerRole::None;
}

void ACrazyRooPlayerController::Server_RequestMoveLeft_Implementation()
{
	if (ASharedCharacterManager* Manager = GetManagerOnServer())
	{
		Manager->RequestMoveLeft(GetMyRoleOnServer());
	}
}

void ACrazyRooPlayerController::Server_RequestMoveRight_Implementation()
{
	if (ASharedCharacterManager* Manager = GetManagerOnServer())
	{
		Manager->RequestMoveRight(GetMyRoleOnServer());
	}
}

void ACrazyRooPlayerController::Server_RequestJump_Implementation()
{
	if (ASharedCharacterManager* Manager = GetManagerOnServer())
	{
		Manager->RequestJump(GetMyRoleOnServer());
	}
}

void ACrazyRooPlayerController::Server_RequestSlide_Implementation()
{
	if (ASharedCharacterManager* Manager = GetManagerOnServer())
	{
		Manager->RequestSlide(GetMyRoleOnServer());
	}
}

void ACrazyRooPlayerController::Server_RequestRotateCameraLeft_Implementation()
{
	if (ASharedCharacterManager* Manager = GetManagerOnServer())
	{
		Manager->RequestRotateCameraLeft(GetMyRoleOnServer());
	}
}

void ACrazyRooPlayerController::Server_RequestRotateCameraRight_Implementation()
{
	if (ASharedCharacterManager* Manager = GetManagerOnServer())
	{
		Manager->RequestRotateCameraRight(GetMyRoleOnServer());
	}
}

void ACrazyRooPlayerController::Server_RequestFireAimRay_Implementation(
	FVector_NetQuantize AimOrigin,
	FVector_NetQuantizeNormal AimDirection)
{
	if (ASharedCharacterManager* Manager = GetManagerOnServer())
	{
		// [한길] 0710 발사방식 선택모드로 인한 변경.
		Manager->RequestFireAimRay(GetMyRoleOnServer(), AimOrigin, AimDirection, this);
	}
}

// [한길] 0703 테스트용 앞뒤 움직임.
void ACrazyRooPlayerController::InputMove(const FInputActionValue& Value)
{
	const float AxisValue = Value.Get<float>();

	if (FMath::IsNearlyZero(AxisValue))
	{
		return;
	}

	Server_RequestMove(AxisValue);
}
void ACrazyRooPlayerController::Server_RequestMove_Implementation(float AxisValue)
{
	if (ASharedCharacterManager* Manager = GetManagerOnServer())
	{
		Manager->RequestMove(AxisValue);
	}
}

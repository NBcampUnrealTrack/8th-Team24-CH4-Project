// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CrazyRooPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class URailInputConfig; //이름 변경 할 수도? 일단 테스트
class ASharedCharacterManager; //이름 변경 할 수도? 일단 테스트
class UCrazyRooCountdownWidget; // [한길] 0711 카운트다운 위젯
enum class ECrazyRooPlayerRole : uint8;
struct FInputActionValue; // 테스트용 앞 뒤 움직임.

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCrazyRooRoleRejected, ECrazyRooPlayerRole, DeniedRole);

UCLASS(abstract)
class ACrazyRooPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** 공유 캐릭터 조작용 입력 에셋 묶음 (DA_RailInputConfig) */
	UPROPERTY(EditDefaultsOnly, Category="Input|Rail")
	URailInputConfig* InputConfig;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

public:

	// 역할 변경 상호작용파트
	// UI 버튼이 클릭되었을 때 호출하는 함수
	UFUNCTION(BlueprintCallable, Category = "CrazyRoo|Lobby")
	void RequestRole(ECrazyRooPlayerRole DesiredRole);

	// 준비완료 버튼
	UFUNCTION(BlueprintCallable, Category="CrazyRoo|Lobby")
	void RequestReady(bool bReady);

	// 위젯이 바인딩할 거부 이벤트 (BP AddDynamic)
	UPROPERTY(BlueprintAssignable, Category = "CrazyRoo|Lobby")
	FOnCrazyRooRoleRejected OnRoleRejected;

	// 시작 버튼이 호출. 어느 맵으로 갈지 MapId 로 지정
	UFUNCTION(BlueprintCallable, Category = "CrazyRoo|Lobby")
	void RequestMap(FName MapId);
	//

	// UI(블루프린트)에서 캐러셀 인덱스 변경을 요청할 때 부르는 함수
	UFUNCTION(BlueprintCallable, Category = "CrazyRoo|Lobby")
	void RequestUpdateCarousel(int32 NewIndex);
	//

	//맵 이동 테스트 함수 삭졔 할 수도 있음
	//void Key_TravelToStage();
	//

	// 테스트: 새 맵에 도착했을 때 카메라를 세팅하라는 서버의 지시(Client RPC)
	UFUNCTION(Client, Reliable)
	void Client_SetupGameplayCamera();

	/**
	* 인게임에서 슈터용 조준 커서 모드를 설정합니다.
	* 슈터라면 일반 마우스 아이콘을 숨기고,
	* WBP_AimCursor만 화면에 보이게 합니다.
	*/
	UFUNCTION(BlueprintCallable, Category = "CrazyRoo|Aim Cursor")
	void SetInGameAimCursorMode(bool bShooter);

	/** 메뉴로 돌아갈 때 일반 마우스 커서를 복구합니다. */
	UFUNCTION(BlueprintCallable, Category = "CrazyRoo|Aim Cursor")
	void RestoreDefaultMouseCursor();

protected:

	// [Client] 서버가 이미 선택된 역할이라고 거부하는 함수입니다.
	UFUNCTION(Client, Reliable)
	void ClientRoleRejected(ECrazyRooPlayerRole DeniedRole);

	// [Server] 내 컴퓨터가 서버한테 동작을 한다고 요청하는 함수입니다 (Reliable: 무조건 전송 보장)
	UFUNCTION(Server, Reliable)
	void ServerRequestRole(ECrazyRooPlayerRole DesiredRole);

	// [Server] 클라이언트의 준비 상태 변경 요청을 받아 실제로 서버 측에서 실행되는 RPC 함수입
	UFUNCTION(Server, Reliable)
	void ServerSetReady(bool bReady);

	UFUNCTION(Server, Reliable)
	void ServerTravel(FName MapId);

	// 서버로 전송되는 실제 RPC 함수
	UFUNCTION(Server, Reliable)
	void Server_UpdateCarousel(int32 NewIndex);

	// ── 공유 캐릭터 조작 (RailCoopPlayerController, 한길님 파트) ──

	/** 로컬 입력 콜백 */
	void InputLeft();
	void InputRight();
	void InputJump();
	void InputSlide();
	void InputFire();
	void InputCameraLeft();
	void InputCameraRight();
	// QTE 중 G키 입력을 서버로 전달합니다.
	void InputQTE();

	/** 클라 → 서버 조작 요청 RPC */
	UFUNCTION(Server, Reliable)
	void Server_RequestMoveLeft();

	UFUNCTION(Server, Reliable)
	void Server_RequestMoveRight();

	UFUNCTION(Server, Reliable)
	void Server_RequestJump();

	UFUNCTION(Server, Reliable)
	void Server_RequestSlide();

	UFUNCTION(Server, Reliable)
	void Server_RequestRotateCameraLeft();

	UFUNCTION(Server, Reliable)
	void Server_RequestRotateCameraRight();

	UFUNCTION(Server, Reliable)
	void Server_RequestFireAimRay(FVector_NetQuantize AimOrigin, FVector_NetQuantizeNormal AimDirection);

	// QTE 입력 카운트는 서버에서만 증가시키기 위해 RPC로 전달합니다.
	UFUNCTION(Server, Reliable)
	void Server_RequestQTEPress();

	/** 마우스 화면 위치 → 월드 Ray 변환 */
	bool GetMouseAimRay(FVector& OutOrigin, FVector& OutDirection) const;

	/** 서버에서 GameMode를 통해 Manager 획득 (로비 맵이면 nullptr) */
	ASharedCharacterManager* GetManagerOnServer() const;

	/** [Server] 내 PlayerState에서 역할 조회 (없으면 None) */
	ECrazyRooPlayerRole GetMyRoleOnServer() const;

	// [한길] 0703 테스트용 앞 뒤 움직임.
	void InputMove(const FInputActionValue& Value);
	UFUNCTION(Server, Unreliable)
	void Server_RequestMove(float AxisValue);

	// [한길] 카운트 다운 위젯 (에디터에서 WBP_Countdown을 지정합니다.)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CrazyRoo|UI")
	TSubclassOf<UCrazyRooCountdownWidget> CountdownWidgetClass;

	// 현재 화면에 생성된 카운트다운 위젯입니다.
	UPROPERTY()
	TObjectPtr<UCrazyRooCountdownWidget> CountdownWidget;

	//[서희] 조준점 UI
	/** 일정 간격마다 현재 슈터의 Aim Ray를 서버로 보냅니다. */
	void SendSharedAimUpdate();

	/** 조준점 공유는 연출 데이터이므로 Unreliable을 사용합니다. */
	UFUNCTION(Server, Unreliable)
	void Server_UpdateSharedAim(
		FVector_NetQuantize AimOrigin,
		FVector_NetQuantizeNormal AimDirection
	);

	/** 서버 전송 주기: 0.05초 = 초당 약 20회 */
	UPROPERTY(EditDefaultsOnly, Category = "CrazyRoo|Aim Cursor")
	float SharedAimUpdateInterval = 0.05f;

	FTimerHandle SharedAimUpdateTimerHandle;
};

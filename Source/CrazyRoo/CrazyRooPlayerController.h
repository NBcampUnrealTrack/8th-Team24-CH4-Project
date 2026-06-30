// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CrazyRooPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
enum class ECrazyRooPlayerRole : uint8;
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

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

public:

	//테스트 용 UI 호출 함수
	// UI 버튼이 클릭되었을 때 호출하는 함수
	UFUNCTION(BlueprintCallable, Category = "CrazyRoo|Lobby")
	void RequestRole(ECrazyRooPlayerRole DesiredRole);

	// 위젯이 바인딩할 거부 이벤트 (BP AddDynamic)
	UPROPERTY(BlueprintAssignable, Category = "CrazyRoo|Lobby")
	FOnCrazyRooRoleRejected OnRoleRejected;

	/** 역할 선택 위젯 클래스 */
	UPROPERTY(EditAnywhere, Category = "CrazyRoo|UI")
	TSubclassOf<UUserWidget> RoleSelectWidgetClass;

	/** 화면에 띄워진 역할 선택 위젯의 인스턴스 */
	UPROPERTY()
	TObjectPtr<UUserWidget> RoleSelectWidget;

	// "시작" 버튼이 호출. 어느 맵으로 갈지 MapId 로 지정
	UFUNCTION(BlueprintCallable, Category = "CrazyRoo|Lobby")
	void RequestMap(FName MapId);

	//

protected:

	// [Client] 서버가 이미 선택된 역할이라고 거부하는 함수입니다.
	UFUNCTION(Client, Reliable)
	void ClientRoleRejected(ECrazyRooPlayerRole DeniedRole);

	// [Server] 내 컴퓨터가 서버한테 동작을 한다고 요청하는 함수입니다 (Reliable: 무조건 전송 보장)
	UFUNCTION(Server, Reliable)
	void ServerRequestRole(ECrazyRooPlayerRole DesiredRole);

	UFUNCTION(Server, Reliable)
	void ServerTravel(FName MapId);
};

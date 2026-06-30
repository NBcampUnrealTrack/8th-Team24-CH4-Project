// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CrazyRooTypes.h"
#include "GameFramework/PlayerController.h"
#include "CrazyRooPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
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
	// UI 버튼이 호출
	UFUNCTION(BlueprintCallable, Category = "CrazyRoo|Lobby")
	void RequestRole(ECrazyRooPlayerRole DesiredRole);

	// 위젯이 바인딩할 거부 이벤트 (BP AddDynamic)
	UPROPERTY(BlueprintAssignable, Category = "CrazyRoo|Lobby")
	FOnCrazyRooRoleRejected OnRoleRejected;

	//테스트용
	/** 역할 선택 위젯 클래스 (BP 에서 WBP_RoleSelect 지정) */
	UPROPERTY(EditAnywhere, Category = "CrazyRoo|UI")
	TSubclassOf<UUserWidget> RoleSelectWidgetClass;

	/** 생성된 역할 선택 위젯 인스턴스 */
	UPROPERTY()
	TObjectPtr<UUserWidget> RoleSelectWidget;
	//

protected:
	UFUNCTION(Server, Reliable)
	void ServerRequestRole(ECrazyRooPlayerRole DesiredRole);

	UFUNCTION(Client, Reliable)
	void ClientRoleRejected(ECrazyRooPlayerRole DeniedRole);

};

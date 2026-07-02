//
// // RailCoopPlayerController.h
//
// #pragma once
//
// #include "CoreMinimal.h"
// #include "GameFramework/PlayerController.h"
// #include "LHG/Types/RailTypes.h"
// #include "RailCoopPlayerController.generated.h"
//
// class UInputMappingContext;
// class UInputAction;
// class ASharedCharacterManager;
// class URailInputConfig;
//
// UCLASS()
// class CRAZYROO_API ARailCoopPlayerController : public APlayerController
// {
// 	GENERATED_BODY()
//
// public:
// 	ARailCoopPlayerController();
//
// 	virtual void BeginPlay() override;
// 	virtual void SetupInputComponent() override;
//
// 	// GameMode가 역할을 배정할 때 호출
// 	void SetPlayerRole(ERailPlayerRole NewRole);
//
// 	// 클라이언트 UI 표시용
// 	UPROPERTY(Replicated, BlueprintReadOnly)
// 	ERailPlayerRole PlayerRole;
//
// protected:
// 	virtual void GetLifetimeReplicatedProps(
// 		TArray<FLifetimeProperty>& OutLifetimeProps
// 	) const override;
//
// 	/**
// 		 * 입력 에셋들을 모아둔 Data Asset
// 		 * 에디터에서 DA_RailInputConfig를 넣어준다
// 	UPROPERTY(EditDefaultsOnly,Category="Input")
// 	URailInputConfig* InputConfig;
//
// 	void InputLeft();
// 	void InputRight();
// 	void InputJump();
// 	void InputSlide();
// 	void InputFire();
// 	void InputCameraLeft();
// 	void InputCameraRight();
//
// 	// 클라이언트에서 호출하지만 서버에서 실행되는 함수들
// 	UFUNCTION(Server, Reliable)
// 	void Server_RequestMoveLeft();
//
// 	UFUNCTION(Server, Reliable)
// 	void Server_RequestMoveRight();
//
// 	UFUNCTION(Server, Reliable)
// 	void Server_RequestJump();
//
// 	UFUNCTION(Server, Reliable)
// 	void Server_RequestSlide();
//
// 	UFUNCTION(Server, Reliable)
// 	void Server_RequestFireToLocation(FVector TargetLocation);
//
// 	bool GetMouseWorldTarget(FVector& OutTargetLocation) const;
//
// 	UFUNCTION(Server, Reliable)
// 	void Server_RequestRotateCameraLeft();
//
// 	UFUNCTION(Server, Reliable)
// 	void Server_RequestRotateCameraRight();
//
// 	UFUNCTION(Server, Reliable)
// 	void Server_RequestFireAimRay(FVector_NetQuantize AimOrigin, FVector_NetQuantizeNormal AimDirection);
//
// 	bool GetMouseAimRay(FVector& OutOrigin, FVector& OutDirection) const;
//
// 	ASharedCharacterManager* GetManagerOnServer() const;
// };
//

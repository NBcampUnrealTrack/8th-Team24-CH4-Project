// SharedRailCharacter.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SharedRailCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;

UCLASS()
class CRAZYROO_API ASharedRailCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ASharedRailCharacter();

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	UCameraComponent* FollowCamera;

	// 현재 레일 번호
	// 0 = 왼쪽, 1 = 중앙, 2 = 오른쪽
	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 CurrentLane;

	// 레일 간격
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rail")
	float LaneDistance = 300.f;

	// 레일 변경 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rail")
	float LaneInterpSpeed = 10.f;

	// 카메라 회전 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	float CameraInterpSpeed = 8.f;

	// 총 사거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shooting")
	float FireRange = 5000.f;

	// 총 데미지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shooting")
	float FireDamage = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shooting")
	float AimRayDistance = 10000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shooting")
	float MuzzleHeight = 80.f;

	// 왼쪽 레일로 이동
	void MoveLeft();

	// 오른쪽 레일로 이동
	void MoveRight();

	// 점프
	void DoJump();

	// 슬라이딩
	void DoSlide();

	// Q 입력
	void RotateCameraLeft();

	// E 입력
	void RotateCameraRight();

	// 사격
	void FireToLocation(const FVector& TargetLocation);

	void FireByAimRay(
	const FVector& AimOrigin,
	const FVector& AimDirection
);

	// 현재 위치를 새로운 중앙 레일로 갱신
	void UpdateLaneCenter();

protected:
	virtual void BeginPlay() override;

	// 레플리케이션 변수 등록
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps
	) const override;

	// 현재 레일 번호에 맞는 목표 위치 계산
	FVector GetTargetLaneLocation() const;

	// 캐릭터가 처음 배치된 위치를 기준점으로 삼게함.
	UPROPERTY()
	FVector StartLocation;

	// 슬라이딩 상태
	UPROPERTY(Replicated)
	bool bIsSliding = false;

	FTimerHandle SlideTimerHandle;

	void EndSlide();

	// 현재 목표 카메라 Yaw
	UPROPERTY(Replicated)
	float TargetCameraYaw = 0.f;

	// 중앙 레일의 기준 위치
	// 캐릭터가 회전하거나 갈림길을 통과할 때 갱신됨
	UPROPERTY(Replicated)
	FVector LaneCenterLocation;

};

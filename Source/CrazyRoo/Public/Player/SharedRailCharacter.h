// SharedRailCharacter.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "RunnerTypes.h"
#include "SharedRailCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class ARailProjectile;

// [한길] 0706 자동전진 / 수동전진 0707 멈춤 추가.
UENUM(BlueprintType)
enum class ERunnerMoveMode : uint8
{
	Stop UMETA(DisplayName = "Stop"),
	AutoMove UMETA(DisplayName = "Auto Move"),
	ManualMove UMETA(DisplayName = "Manual Move")
};

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
	float LaneDistance = 450.f;

	// 레일 변경 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rail")
	float LaneInterpSpeed = 10.f;

	// 카메라 회전 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	float CameraInterpSpeed = 8.f;

	// [한길] 0703 카메라 각도 변경.
	// 카메라 각도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	float CameraPitch = -20.f;

	// 총 사거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shooting")
	float FireRange = 800.f;

	// 총 데미지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shooting")
	float FireDamage = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shooting")
	float AimRayDistance = 10000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shooting")
	float MuzzleHeight = 80.f;

	// ── 플레이어(공유 캐릭터) 체력 ──
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HP", meta=(ClampMin="1.0"))
	float MaxPlayerHP = 10.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="HP")
	float CurrentPlayerHP = 10.f;

	// 무적 상태 복제 (나중에 클라이언트에서 피격 후 후처리 켤 때 사용)
	UPROPERTY(Replicated, BlueprintReadOnly, Category="HP")
	bool bIsInvincible = false;

	// 무적 유지 시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HP")
	float InvincibilityDuration = 1.0f;

	FTimerHandle InvincibilityTimerHandle;

	// 에디터에서 방금 만든 UpperBody 펀치 몽타주를 할당할 변수
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Animation")
	UAnimMontage* PunchMontage;

	// 몽타주를 모든 클라이언트에서 재생하기 위한 멀티캐스트 함수 선언
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayPunchMontage();

	// QTE 입력 성공 시 재생할 전용 몽타주입니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Animation|QTE")
	UAnimMontage* QTEPunchMontage;

	// QTE 전용 몽타주를 모든 클라이언트에서 재생합니다.
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayQTEPunchMontage();

	// 무적 상태 해제
	void ResetInvincibility();

	// 왼쪽 레일로 이동
	void MoveLeft();

	// 오른쪽 레일로 이동
	void MoveRight();

	// 현재 레인을 가운데 레인으로 되돌립니다.
	void MoveToCenterLane();

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
	// 나중에 플레이 해보고 빼도 되는 기능.
	void UpdateLaneCenter();

	// [한길] 0703 코너링 시스템 추가
	// 코너링이 성공했을 때 캐릭터를 지정된 각도만큼 회전시킵니다.
	// 예) 좌회전 : -90°, 우회전 : +90°
	void ApplyCornerTurn(float YawDelta);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void ApplyCornerTurnAtSnapPoint(float YawDelta, const FTransform& SnapTransform);

	// [한길] 0703 테스트용 앞뒤 움직임. 0706 자동 전진 / 수동 전진 구분.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Replicated, Category="Movement")
	ERunnerMoveMode MoveMode = ERunnerMoveMode::Stop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Auto")
	float AutoForwardMoveSpeed = 1400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Debug")
	float DebugMoveSpeed = 600.f;
	void DebugMoveForward(float AxisValue);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Cornering", meta = (ClampMin = "0.0", Units = "s"))
	float CornerSnapDuration = 0.2f;

	// [한길] 0707 MoveMode 변경 함수.
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetMoveMode(ERunnerMoveMode NewMoveMode);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Debug")
	bool bDebugLaneCenterTracking = false;

	/** [Server] 적/장애물이 플레이어에게 데미지를 줄 때 호출 */
	void ApplyPlayerDamage(float Damage, AActor* DamageCauser, bool bIgnoreInvincibility);

	UFUNCTION(BlueprintPure, Category="HP")
	bool IsPlayerDead() const { return CurrentPlayerHP <= 0.f; }
	//

	// [한길] 0713
	// 캡슐 크기를 바꾸지 않고 슬라이딩 회피 판정에 사용할 현재 슬라이딩 상태입니다.
	UFUNCTION(BlueprintPure, Category="Movement")
	bool IsSliding() const { return bIsSliding; }

	UFUNCTION(BlueprintPure, Category="Rail")
	FVector GetLaneCenterLocation() const;

	UFUNCTION(BlueprintPure, Category="Rail")
	FTransform GetLaneCenterTransform() const;

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

	bool bIsCornerSnapInterpolating = false;
	float CornerSnapElapsedTime = 0.0f;
	FVector CornerSnapStartLocation = FVector::ZeroVector;
	FVector CornerSnapTargetLocation = FVector::ZeroVector;
	float PendingCornerYawDelta = 0.0f;

	// 중앙 레일의 기준 위치
	// 캐릭터가 회전하거나 갈림길을 통과할 때 갱신됨
	UPROPERTY(Replicated)
	FVector LaneCenterLocation;

	void FinishCornerSnapTurn();

	/** [Server] HP 0 도달 시 1회 실행. 지금은 로그만, 나중에 게임오버 연결 지점 */
	void HandlePlayerDeath();

	/** [Server→All] 사망 사실을 모든 클라 화면에 표시 */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyPlayerDeath();

	// 사망 처리 중복 방지
	bool bPlayerDeathHandled = false;
	float DebugLaneCenterTrackingLogElapsed = 0.0f;

// [한길] 0710 발사 방식 변경.
public:
	// 현재 사용할 발사 방식
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shooting")
	ERunnerFireMode FireMode = ERunnerFireMode::LineTrace;

	// 스폰할 Projectile 블루프린트 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shooting|Projectile")
	TSubclassOf<ARailProjectile> ProjectileClass;

	// 투사체 데미지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shooting|Projectile")
	float ProjectileDamage = 20.f;

	// 캐릭터 중심으로부터 총구 높이
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shooting|Projectile")
	float ProjectileSpawnHeight = 80.f;

	// 캐릭터 중심에서 앞으로 얼마나 떨어져 스폰할지 설정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shooting|Projectile")
	float ProjectileSpawnForwardOffset = 100.f;

	/**
	 * 현재 FireMode에 따라 LineTrace 또는 Projectile 방식으로 발사합니다.
	 */
	void FireBySelectedMode(
		const FVector& AimOrigin,
		const FVector& AimDirection,
		AController* ShooterController
	);

	/**
	 * Projectile 방식의 실제 발사 함수입니다.
	 */
	void FireProjectileByAimRay(
		const FVector& AimOrigin,
		const FVector& AimDirection,
		AController* ShooterController
	);
};

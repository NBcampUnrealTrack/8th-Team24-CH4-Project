// CrazyRoo - System Framework & Multiplayer
// Shared enums, structs and delegate declarations used across the framework.

#pragma once

#include "CoreMinimal.h"
#include "CrazyRooTypes.generated.h"

/**
 *  비대칭 조작 역할 Enum
 *  4명의 플레이어가 하나의 캐릭터를 나눠서 조작한다.
 *  - P_Left   : 왼쪽 레인 이동 + 좌회전 코너링 타이밍
 *  - P_Right  : 오른쪽 레인 이동 + 우회전 코너링 타이밍
 *  - P_Shooter: 마우스 에임 사격 + 카메라 회전
 *  - P_Acrobat: 점프(상단 회피) / 숙이기(하단 회피)
 */
UENUM(BlueprintType)
enum class ECrazyRooPlayerRole : uint8
{
	None		UMETA(DisplayName = "Unassigned"),
	P_Left		UMETA(DisplayName = "Left Mover"),
	P_Right	UMETA(DisplayName = "Right Mover"),
	P_Shooter	UMETA(DisplayName = "Shooter"),
	P_Acrobat	UMETA(DisplayName = "Acrobat")
};

/** 전체 게임 진행 상태 enum. GameState에 복제되어 모든 클라이언트가 공유 */
UENUM(BlueprintType)
enum class ECrazyRooGameState : uint8
{
	WaitingForPlayers	UMETA(DisplayName = "Waiting For Players"),	    // 인원 기다리는 상태
	RoleSelection		UMETA(DisplayName = "Role Selection"),			// 역할 선택 및 준비
	Countdown			UMETA(DisplayName = "Countdown"),				// 시작 타이머
	Running				UMETA(DisplayName = "Running"),				    // 게임 중
	GameOver			UMETA(DisplayName = "Game Over"),				// 게임 오버
	Result				UMETA(DisplayName = "Result")					// 스테이지 결과
};

/** 3개 레인 중 현재 위치. */
UENUM(BlueprintType)
enum class ECrazyRooLane : uint8
{
	Left	UMETA(DisplayName = "Left"),
	Center	UMETA(DisplayName = "Center"),
	Right	UMETA(DisplayName = "Right")
};

/** 카메라 방향 전환 방향. Shooter가 카메라 상태를 변경 가능하게 함 */
UENUM(BlueprintType)
enum class ECrazyRooCameraDirection : uint8
{
	None	UMETA(DisplayName = "None"),
	Left	UMETA(DisplayName = "Left"),
	Right	UMETA(DisplayName = "Right")
};

/** 상/하단 회피 입력 상태. */
UENUM(BlueprintType)
enum class ECrazyRooDodgeType : uint8
{
	None	UMETA(DisplayName = "None"),
	Jump	UMETA(DisplayName = "Jump"),
	Duck	UMETA(DisplayName = "Duck")
};

/** 스테이지 선택 정보. GameInstance에 보관되어 레벨 이동 시 유지된다. */
USTRUCT(BlueprintType)
struct FCrazyRooStageInfo
{
	GENERATED_BODY()

	/** 스테이지 고유 ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
	FName StageId = NAME_None;

	/** UI에 표시할 맵 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
	FText DisplayName;

	/** 읽어들일 실제 맵 (소프트 레퍼런스) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
	TSoftObjectPtr<UWorld> Level;

	/** 해당 맵의 기본 주행 속도 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
	float StageBaseRunSpeed = 1200.f;
};

// ---------------------------------------------------------------------------
//  UI / 게임플레이 바인딩용 동적 멀티캐스트 델리게이트
// ---------------------------------------------------------------------------

/** 매치 상태가 바뀔 때 (GameState) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCrazyRooMatchStateChanged, ECrazyRooGameState, NewState);

/** 역할 선택(로비)에서 역할이 바뀔 때 (PlayerState) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCrazyRooRoleChanged, ECrazyRooPlayerRole, NewRole);

/** 공유 HP 가 바뀔 때 (GameState) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCrazyRooHealthChanged, int32, NewHP, int32, MaxHP);

/** 카운트다운 숫자가 바뀔 때 (GameState) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCrazyRooCountdownChanged, int32, SecondsLeft);

/** 플레이어가 준비 하거나 취소를 해서 상태를 바꿨을 때 (PlayerState) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCrazyRooReadyChanged, bool, bIsReady);

/** 주행 거리나 점수가 올라갔을 때*/
//DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCrazyRooScoreChanged, float, RunDistance, int32, Score);

// CrazyRoo - System Framework & Multiplayer
// Shared enums, structs and delegate declarations used across the framework.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
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

/** 스테이지를 어딜 갈지 저장하는 구조체 (데이터 테이블 표에 사용되는 구조체입니다) */
USTRUCT(BlueprintType)
struct FCrazyRooStageInfo : public FTableRowBase
{
	GENERATED_BODY()

	/** 맵 식별자 (예: "Title", "Select", "Stage1") — 코드에서 이걸로 찾음 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
	FName MapId = NAME_None;

	/** UI에 띄워줄 맵 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
	FText DisplayName;

	/** 읽어들일 실제 맵 (소프트 레퍼런스 - 파일 경로가 바뀌어도 안전함) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
	TSoftObjectPtr<UWorld> Level;
};


/** 전체 게임 진행 상태 enum. GameState에 복제되어 모든 클라이언트가 공유 */
//UENUM(BlueprintType)
//enum class ECrazyRooGameState : uint8
//{
//	WaitingForPlayers	UMETA(DisplayName = "Waiting For Players"),	    // 인원 기다리는 상태
//	RoleSelection		UMETA(DisplayName = "Role Selection"),			// 역할 선택 및 준비
//	Countdown			UMETA(DisplayName = "Countdown"),				// 시작 타이머
//	Running				UMETA(DisplayName = "Running"),				    // 게임 중
//	GameOver			UMETA(DisplayName = "Game Over"),				// 게임 오버
//	Result				UMETA(DisplayName = "Result")					// 스테이지 결과
//};

/** 3개 레인 중 현재 위치. */
//UENUM(BlueprintType)
//enum class ECrazyRooLane : uint8
//{
//	Left	UMETA(DisplayName = "Left"),
//	Center	UMETA(DisplayName = "Center"),
//	Right	UMETA(DisplayName = "Right")
//};

/** 카메라 방향 전환 방향. Shooter가 카메라 상태를 변경 가능하게 함 */
//UENUM(BlueprintType)
//enum class ECrazyRooCameraDirection : uint8
//{
//	None	UMETA(DisplayName = "None"),
//	Left	UMETA(DisplayName = "Left"),
//	Right	UMETA(DisplayName = "Right")
//};

/** 상/하단 회피 입력 상태. */
//ENUM(BlueprintType)
//num class ECrazyRooDodgeType : uint8
//
//	None	UMETA(DisplayName = "None"),
//	Jump	UMETA(DisplayName = "Jump"),
//	Duck	UMETA(DisplayName = "Duck")
//;


// 비대칭 조작 역할 Enum을 문자열 변환 (구현은 CrazyRooTypes.cpp)
const TCHAR* CrazyRooRoleToString(ECrazyRooPlayerRole R);

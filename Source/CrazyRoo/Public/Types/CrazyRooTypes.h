// CrazyRoo - System Framework & Multiplayer
// Shared enums, structs and delegate declarations used across the framework.

#pragma once

#include "CoreMinimal.h"
#include "Audio/CrazyRooAudioTypes.h"
#include "Engine/DataTable.h"
#include "CrazyRooTypes.generated.h"

class UUserWidget;
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

/** [서희]
 * 인게임 HUD에 표시할 공용 조작 키입니다.
 *
 * 키 입력은 각 플레이어의 PlayerController에서 시작되지만,
 * 승인된 입력은 GameState를 통해 모든 클라이언트 화면에 전달됩니다.
 */
UENUM(BlueprintType)
enum class ECrazyRooSharedKey : uint8
{
	Shift UMETA(DisplayName = "Shift"),
	Space UMETA(DisplayName = "Space"),
	Q     UMETA(DisplayName = "Q"),
	Mouse UMETA(DisplayName = "Mouse"),
	E     UMETA(DisplayName = "E"),
	A     UMETA(DisplayName = "A"),
	D     UMETA(DisplayName = "D")
};

// [한길] 0712
// 게임플레이 맵이 시작될 때 어떤 흐름으로 이동을 시작할지 결정합니다.
// ERunnerMoveMode는 캐릭터의 실제 이동 방식이고, 이 enum은 테스트/실게임 시작 절차를 고르는 용도입니다.
UENUM(BlueprintType)
enum class ECrazyRooGameStartMode : uint8
{
	// 실게임 기본 흐름: 카운트다운 후 자동 전진을 시작합니다.
	Normal UMETA(DisplayName = "Normal"),

	// 테스트용 흐름: 카운트다운 없이 바로 수동 전/후진 입력을 받습니다.
	ManualTest UMETA(DisplayName = "Manual Test"),

	// 테스트용 흐름: 카운트다운 없이 바로 자동 전진을 시작합니다.
	AutoMoveTest UMETA(DisplayName = "Auto Move Test")
};

/** 스테이지를 어딜 갈지 저장하는 구조체 (데이터 테이블 표에 사용되는 구조체입니다) */
USTRUCT(BlueprintType)
struct FCrazyRooStageInfo : public FTableRowBase
{
	GENERATED_BODY()

	/** 읽어들일 실제 맵 (소프트 레퍼런스 - 파일 경로가 바뀌어도 안전함) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
	TSoftObjectPtr<UWorld> Level;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSoftClassPtr<UUserWidget> StageUIClass;

	/** 이 맵이 실제 게임플레이 맵인지 (true면 SharedCharacterManager 스폰) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
	bool bIsGameplayStage = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	ECrazyRooBGMType BGMType = ECrazyRooBGMType::None;
};


/** * 방 목록 UI 한 줄(Row)에 대응하는 정보 구조체
 * (언리얼의 복잡한 SearchResults 데이터를 블루프린트 UI에서 쉽게 꺼내 쓸 수 있도록 가공한 형태)
 */
USTRUCT(BlueprintType)
struct FCrazyRooRoomInfo
{
	GENERATED_BODY()

	// 유저가 UI에서 [참가] 버튼을 눌렀을 때, JoinRoomByIndex 에 넘겨줄 고유 인덱스
	UPROPERTY(BlueprintReadOnly, Category = "CrazyRoo|Session")
	int32 SearchResultIndex = -1;

	// UI에 표시할 방 이름 (예: "CrazyRoo Room")
	UPROPERTY(BlueprintReadOnly, Category = "CrazyRoo|Session")
	FString RoomName;

	//현재 방에 들어와 있는 유저 수
	UPROPERTY(BlueprintReadOnly, Category = "CrazyRoo|Session")
	int32 CurrentPlayers = 0;

	// 방의 최대 수용 인원
	UPROPERTY(BlueprintReadOnly, Category = "CrazyRoo|Session")
	int32 MaxPlayers = 0;

	// 서버와의 통신 지연율(Ping)
	UPROPERTY(BlueprintReadOnly, Category = "CrazyRoo|Session")
	int32 PingMs = 0;
};

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

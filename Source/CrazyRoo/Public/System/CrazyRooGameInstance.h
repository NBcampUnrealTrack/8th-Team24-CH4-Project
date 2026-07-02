// CrazyRoo 게임 인스턴스 클래스
#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "CrazyRooGameInstance.generated.h"

enum class ECrazyRooPlayerRole : uint8;
class UUserWidget;
struct FCrazyRooStageInfo;

UCLASS()
class CRAZYROO_API UCrazyRooGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// 기본 생성자
	UCrazyRooGameInstance();

	virtual void Init() override;

	// 플레이어 순번 확인용
	UFUNCTION(BlueprintCallable, Category = "CrazyRoo|Role")
	void SavePlayerJoinOrder(int32 PlayerId, int32 JoinOrder);

	UFUNCTION(BlueprintCallable, Category = "CrazyRoo|Role")
	int32 GetSavedPlayerJoinOrder(int32 PlayerId) const;
	//

	// 특정 PlayerId가 로비에서 무슨 역할을 골랐는지 Map에 저장하는 함수
	UFUNCTION(BlueprintCallable, Category = "CrazyRoo|Role")
	void SavePlayerRole(int32 PlayerId, ECrazyRooPlayerRole Role);

	// 레벨 이동 후, 해당 플레이어(PlayerId)의 역할 반환
	UFUNCTION(BlueprintCallable, Category = "CrazyRoo|Role")
	ECrazyRooPlayerRole GetSavedPlayerRole(int32 PlayerId) const;

	// 저장된 역할 초기화하는 함수입니다.
	UFUNCTION(BlueprintCallable, Category = "CrazyRoo|Role")
	void ClearSavedRoles();

	//맵 정보 저장하는 곳
	/** 맵 데이터 테이블  */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Map")
	UDataTable* MapDataTable;

	/** 맵 식별자(MapId 또는 Row Name)를 주면 데이터 테이블에서 맵의 절대 경로 문자열을 뽑아주는 헬퍼 */
	FString GetStageMapPath(FName MapId) const;

	/** MapId(Row Name)로 데이터 테이블 행 전체를 찾아 반환. 못 찾으면 nullptr */
	const FCrazyRooStageInfo* FindStageRow(FName MapId) const;

	/**  도착한 맵의 MapId. 트래블 시 저장, 도착 후 MatchState 결정에 사용 */
	UPROPERTY()
	FName CurrentMapId = TEXT("TestMap");//여기에 초기값을 할당해서 바로 처음 나오는 맵을 나오게 가능, NAME_None

	/** 실제 월드 이름으로 데이터 테이블에서 MapId(RowName) 역조회. 못 찾으면 NAME_None */
	FName FindMapIdByWorldName(const FString& WorldName) const;

protected:
	// 플레이어의 고유 ID와 선택한 역할을 저장하는 Map
	UPROPERTY()
	TMap<int32, ECrazyRooPlayerRole> SavedRoles;

	UPROPERTY()
	TMap<int32, int32> SavedJoinOrders;
};

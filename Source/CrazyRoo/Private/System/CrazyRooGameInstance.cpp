// CrazyRoo 게임 인스턴스 클래스
#include "System/CrazyRooGameInstance.h"
#include "Types/CrazyRooTypes.h"
#include "CrazyRoo.h"
#include "Blueprint/UserWidget.h"

UCrazyRooGameInstance::UCrazyRooGameInstance()
{

}

void UCrazyRooGameInstance::Init()
{
	Super::Init();
	// GameInstance 는 프로세스마다 1개. World가 아직 없을 수 있어 Print 대신 UE_LOG 직접 사용.
	UE_LOG(LogCrazyRoo, Warning, TEXT("GameInstance Init (프로세스마다 1개)"));
}

void UCrazyRooGameInstance::SavePlayerJoinOrder(int32 PlayerId, int32 JoinOrder)
{
	SavedJoinOrders.Add(PlayerId, JoinOrder);
	UE_LOG(LogCrazyRoo, Warning, TEXT("[GI] SaveJoinOrder Id=%d -> %d"), PlayerId, JoinOrder);
}

int32 UCrazyRooGameInstance::GetSavedPlayerJoinOrder(int32 PlayerId) const
{
	const int32* Found = SavedJoinOrders.Find(PlayerId);
	return Found ? *Found : 0;
}

void UCrazyRooGameInstance::SavePlayerRole(int32 PlayerId, ECrazyRooPlayerRole Role)
{
	// 만약 플레이어가 None(취소) 했다면 Map에서 삭제
	if (Role == ECrazyRooPlayerRole::None)
	{
		SavedRoles.Remove(PlayerId);
	}
	else // 역할을 골랐다면 Map 추가
	{
		SavedRoles.Add(PlayerId, Role);
	}
	UE_LOG(LogCrazyRoo, Warning, TEXT("[GI] SaveRole Id=%d -> %s"),PlayerId, CrazyRooRoleToString(Role));
}

ECrazyRooPlayerRole UCrazyRooGameInstance::GetSavedPlayerRole(int32 PlayerId) const
{
	// Map에서 해당 플레이어 역할을 찾음
	const ECrazyRooPlayerRole* Found = SavedRoles.Find(PlayerId);
	if (Found)
	{
		return *Found;
	}
	// 못찾으면 None을 반환
	return ECrazyRooPlayerRole::None;
}

void UCrazyRooGameInstance::ClearSavedRoles()
{
	//Map 아예 초기화
	SavedRoles.Reset();
}

FString UCrazyRooGameInstance::GetStageMapPath(FName MapId) const
{
	// 데이터 테이블 미지정 방어
	if (!MapDataTable)
	{
		UE_LOG(LogCrazyRoo, Warning, TEXT("[GI] MapDataTable 이 지정되지 않음"));
		return FString();
	}

	// Row Name(=MapId) 으로 행을 바로 찾음 (순회 없이 해시 조회)
	const FCrazyRooStageInfo* Row = FindStageRow(MapId);
	if (!Row)
	{
		UE_LOG(LogCrazyRoo, Warning, TEXT("[GI] 맵 '%s' 를 테이블에서 못 찾음"), *MapId.ToString());
		return FString();
	}

	// 레벨 참조가 비어있는지 확인
	if (Row->Level.IsNull())
	{
		UE_LOG(LogCrazyRoo, Warning, TEXT("[GI] 맵 '%s' 의 Level 이 비어있음"), *MapId.ToString());
		return FString();
	}

	// 소프트 참조 → 트래블에 쓸 절대 경로 문자열로 변환
	const FString Path = Row->Level.ToSoftObjectPath().GetLongPackageName();
	UE_LOG(LogCrazyRoo, Warning, TEXT("[GI] 맵 '%s' 경로 = %s"), *MapId.ToString(), *Path);
	return Path;
}

const FCrazyRooStageInfo* UCrazyRooGameInstance::FindStageRow(FName MapId) const
{
	if (!MapDataTable)
	{
		return nullptr;
	}
	return MapDataTable->FindRow<FCrazyRooStageInfo>(MapId, TEXT("FindStageRow"));
}


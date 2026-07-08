// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "CrazyRooSessionSubsystem.generated.h"

// CrazyRoo 세션 서브시스템
// [핵심 아키텍처] 데디케이티드 서버 1대 = 방 1개 전제.
// - 서버: 부팅 시 자동으로 세션 1개를 온라인/LAN에 등록 하고 대기함.
// - 클라: 방 검색(FindRooms) → 검색된 목록을 UI에 표시 → 유저가 클릭 시 참가(JoinRoomByIndex)
// - 클라의 "방 생성": 실제로 유저가 세션을 만드는 것이 아님. 검색을 먼저 돌려서 비어있는 서버를 찾은 뒤, 첫 번째로 입장하여 '방장' 권한을 얻는 방식.



struct FCrazyRooRoomInfo;

// UI(위젯 BP)가 바인딩할 델리게이트들
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCrazyRooFindRoomsComplete, const TArray<FCrazyRooRoomInfo>&, Rooms, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCrazyRooJoinRoomComplete, bool, bSuccess, const FString&, FailReason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCrazyRooCreateRoomFailed, const FString&, FailReason);

UCLASS(BlueprintType)
class CRAZYROO_API UCrazyRooSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ── 서버 전용 ──────────────────────────────────────────────

	// 데디서버 부팅 시 자동으로 호출되어 세션 1개를 온라인 서비스(또는 LAN)에 등록합니다. 클라이언트는 이 함수를 호출하지 않음
	void CreateDedicatedSession();

	// ── 클라이언트 전용 (타이틀/방목록 UI에서 호출) ─────────────

	//"방 참가하기" 버튼 클릭 시 호출
	//온라인 서브시스템을 통해 켜져있는 서버(세션)들을 검색
	//검색이 완료되면 OnFindRoomsComplete 델리게이트를 통해 UI에 목록 제공
	UFUNCTION(BlueprintCallable, Category = "CrazyRoo|Session")
	void FindRooms();

	// 유저가 방 목록 UI에서 특정 방을 참가했을 때 호출.
	//인덱스를 받아 해당 서버의 IP/Port 주소로 접속(ClientTravel)을 시도합니다.
	UFUNCTION(BlueprintCallable, Category = "CrazyRoo|Session")
	void JoinRoomByIndex(int32 SearchResultIndex);

	/**
		 * "방 생성하기" 버튼 클릭 시 호출.
		 * 데디서버 환경이므로 클라가 직접 방을 파지 않고 내부적으로 bAutoJoinIfEmpty 플래그를 켠 채 방을 검색합니다.
		 * 검색 후:
		 * (1) 켜진 서버가 없으면 -> 실패 알림
		 * (2) 인원이 1명이라도 있으면 -> "이미 방이 존재합니다" 실패 알림
		 * (3) 0명인 빈 방을 찾으면 -> 1등으로 자동 입장시켜서 방장을 만들어줌.
		 */
	UFUNCTION(BlueprintCallable, Category = "CrazyRoo|Session")
	void CreateRoom();

	// ── UI 바인딩용 델리게이트 ─────────────────────────────────

	// 방 검색이 끝났을 때 UI의 이벤트 노드를 실행시킵니다.
	UPROPERTY(BlueprintAssignable, Category = "CrazyRoo|Session")
	FOnCrazyRooFindRoomsComplete OnFindRoomsComplete;

	// 방 참가(JoinRoomByIndex)의 최종 성공/실패 여부를 UI에 제공
	UPROPERTY(BlueprintAssignable, Category = "CrazyRoo|Session")
	FOnCrazyRooJoinRoomComplete OnJoinRoomComplete;

	// 방 생성(CreateRoom) 로직이 실패했을 때 에러 메시지를 UI에 제공
	UPROPERTY(BlueprintAssignable, Category = "CrazyRoo|Session")
	FOnCrazyRooCreateRoomFailed OnCreateRoomFailed;

protected:
	// 온라인 서브시스템 콜백 핸들러 (비동기 작업 완료 시 엔진이 호출해 줌)
	void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleFindSessionsComplete(bool bWasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	// 온라인 세션 인터페이스를 안전하게 가져오는 유틸리티 함수
	IOnlineSessionPtr GetSessionInterface() const;

	// 원본 검색 결과 데이터를 우리가 만든 FCrazyRooRoomInfo 구조체로 만들어주는 함수
	FCrazyRooRoomInfo MakeRoomInfo(int32 Index) const;

	//방의 최대 인원
	static constexpr int32 MaxPlayersPerRoom = 4;

	// 세션 검색 시 사용할 방 이름에 대한 커스텀 키값
	static const FName RoomNameKey;

	// 방 검색을 돌리고 난 뒤, 그 결과 리스트를 들고 있는 변수 (참가할 때 여기서 인덱스로 꺼내 씀)
	TSharedPtr<class FOnlineSessionSearch> SessionSearch;

	// 델리게이트 핸들 (Deinitialize에서 해제)
	FDelegateHandle CreateSessionHandle;
	FDelegateHandle FindSessionsHandle;
	FDelegateHandle JoinSessionHandle;

	/** * "방 생성" 버튼을 눌렀을 때 활성화되는 플래그.
		 * true일 경우 방 검색이 끝난 직후 UI에 리스트를 제공하지않고,
		 * 빈 서버를 스스로 찾아서 접속(Join)을 시도
		 */
	bool bAutoJoinIfEmpty = false;

	// 검색 진행 중 플래그.
	// - FindRooms: 진행 중이면 새 검색을 시작하지 않음 (진행 중인 검색 결과에 편승)
	// - JoinRoomByIndex: 진행 중이면 거부 (검색이 인덱스를 갈아치우는 중이라 참가 불가)
	bool bFindInProgress = false;

};

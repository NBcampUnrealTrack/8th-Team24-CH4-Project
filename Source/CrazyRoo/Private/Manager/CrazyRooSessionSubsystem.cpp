// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/CrazyRooSessionSubsystem.h"
#include "CrazyRoo.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameInstance.h"
#include "Types/CrazyRooTypes.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

const FName UCrazyRooSessionSubsystem::RoomNameKey(TEXT("ROOM_NAME"));
const FName UCrazyRooSessionSubsystem::PlayerCountKey(TEXT("CURPLAYERS")); // 현재 인원수 광고 키

void UCrazyRooSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		UE_LOG(LogCrazyRoo, Error, TEXT("[Session] OnlineSubsystem 세션 인터페이스 없음 — DefaultEngine.ini 확인"));
		return;
	}

	// 콜백 등록 (한 번만)
	CreateSessionHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UCrazyRooSessionSubsystem::HandleCreateSessionComplete));
	FindSessionsHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UCrazyRooSessionSubsystem::HandleFindSessionsComplete));
	JoinSessionHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &UCrazyRooSessionSubsystem::HandleJoinSessionComplete));

	// [디버그] UpdateSession이 백엔드에서 성공하는지 확인
	Sessions->AddOnUpdateSessionCompleteDelegate_Handle(
		FOnUpdateSessionCompleteDelegate::CreateLambda([](FName SessionName, bool bWasSuccessful)
		{
			UE_LOG(LogCrazyRoo, Warning, TEXT("[Session] UpdateSession 완료: %s (%s)"),
				*SessionName.ToString(), bWasSuccessful ? TEXT("성공") : TEXT("실패"));
		}));

	// [EOS] 로그인 완료 콜백 등록 (클라 전용 기능이지만 등록 자체는 무해)
	if (IOnlineIdentityPtr Identity = GetIdentityInterface())
	{
		LoginHandle = Identity->AddOnLoginCompleteDelegate_Handle(0,
			FOnLoginCompleteDelegate::CreateUObject(this, &UCrazyRooSessionSubsystem::HandleLoginComplete));
	}

	// 데디케이티드 서버 프로세스라면 부팅과 동시에 세션 1개 등록
	// [EOS] 서버는 로그인 불필요 — ini의 TrustedServer 자격증명(CrazyRooServer Artifact)으로 인증됨
	if (IsRunningDedicatedServer())
	{
		CreateDedicatedSession();
	}

}

void UCrazyRooSessionSubsystem::Deinitialize()
{
	if (IOnlineSessionPtr Sessions = GetSessionInterface())
	{
		// [정리] 종료 시 내가 만든/참가한 세션 해제 → 유령 방 방지 (정상 종료 한정)
		if (Sessions->GetNamedSession(NAME_GameSession))
		{
			Sessions->DestroySession(NAME_GameSession);
		}
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionHandle);
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsHandle);
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionHandle);
	}

	// [EOS] 로그인 콜백 해제
	if (IOnlineIdentityPtr Identity = GetIdentityInterface())
	{
		Identity->ClearOnLoginCompleteDelegate_Handle(0, LoginHandle);
	}


	Super::Deinitialize();
}

IOnlineSessionPtr UCrazyRooSessionSubsystem::GetSessionInterface() const
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	return OSS ? OSS->GetSessionInterface() : nullptr;
}

IOnlineIdentityPtr UCrazyRooSessionSubsystem::GetIdentityInterface() const
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	return OSS ? OSS->GetIdentityInterface() : nullptr;
}

bool UCrazyRooSessionSubsystem::RequiresLogin() const
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	return OSS && OSS->GetSubsystemName() != NULL_SUBSYSTEM;
}

// ─────────────────────────────────────────────────────────────
// [EOS][Client] 로그인
// ─────────────────────────────────────────────────────────────

void UCrazyRooSessionSubsystem::LoginToEOS()
{
	// 이미 로그인됨(또는 LAN 모드라 불필요) → 즉시 성공 통지
	if (IsLoggedIn())
	{
		OnLoginComplete.Broadcast(true, FString());
		return;
	}

	if (bLoginInProgress)
	{
		UE_LOG(LogCrazyRoo, Warning, TEXT("[Session][CLIENT] 로그인 진행 중 — 중복 요청 무시"));
		return;
	}

	IOnlineIdentityPtr Identity = GetIdentityInterface();
	if (!Identity.IsValid())
	{
		OnLoginComplete.Broadcast(false, TEXT("온라인 서비스 초기화 실패 (ini 설정 확인)"));
		return;
	}

	bLoginInProgress = true;

	// ── [개발 모드] 실행 인자에 -AUTH_LOGIN이 있으면 DevAuthTool 로그인 (bat 파일 경로) ──
	FString Unused;
	if (FParse::Value(FCommandLine::Get(), TEXT("AUTH_LOGIN="), Unused))
	{
		UE_LOG(LogCrazyRoo, Warning, TEXT("[Session][CLIENT] EOS AutoLogin 시작 (DevAuthTool)"));
		if (!Identity->AutoLogin(0))
		{
			bLoginInProgress = false;
			OnLoginComplete.Broadcast(false, TEXT("로그인을 시작할 수 없습니다. 실행 인자(-AUTH_*)를 확인하세요."));
		}
		return;
	}

	// ── [배포 모드] 인자가 없으면: 항상 에픽 로그인 창(accountportal)으로 ──
	UE_LOG(LogCrazyRoo, Warning, TEXT("[Session][CLIENT] EOS AccountPortal 로그인 시도"));
	FOnlineAccountCredentials Credentials(TEXT("accountportal"), TEXT(""), TEXT(""));
	Identity->Login(0, Credentials);

	// ── [배포 모드] 인자가 없으면: 저장된 토큰(persistentauth)으로 로그인 시도 ──
	//UE_LOG(LogCrazyRoo, Warning, TEXT("[Session][CLIENT] EOS PersistentAuth 로그인 시도"));
	//bTriedPersistentAuth = true;
	//FOnlineAccountCredentials Credentials(TEXT("persistentauth"), TEXT(""), TEXT(""));
	//Identity->Login(0, Credentials);

}

bool UCrazyRooSessionSubsystem::IsLoggedIn() const
{
	// LAN(Null) 모드에서는 로그인이 필요 없으므로 항상 true 취급 → BP 흐름을 EOS/LAN 공용으로 유지
	if (!RequiresLogin())
	{
		return true;
	}

	IOnlineIdentityPtr Identity = GetIdentityInterface();
	return Identity.IsValid() && Identity->GetLoginStatus(0) == ELoginStatus::LoggedIn;
}

void UCrazyRooSessionSubsystem::HandleLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId,
	const FString& Error)
{
	// [배포] persistentauth 실패 = 최초 실행이거나 토큰 만료 → 에픽 로그인 창으로 폴백 (1회만)
	//if (!bWasSuccessful && bTriedPersistentAuth)
	//{
	//	bTriedPersistentAuth = false;
	//	UE_LOG(LogCrazyRoo, Warning, TEXT("[Session][CLIENT] PersistentAuth 실패 → AccountPortal 폴백"));

	//	if (IOnlineIdentityPtr Identity = GetIdentityInterface())
	//	{
	//		FOnlineAccountCredentials Credentials(TEXT("accountportal"), TEXT(""), TEXT(""));
	//		Identity->Login(0, Credentials);
	//		return; // 다음 콜백을 기다림 (bLoginInProgress는 유지)
	//	}
	//}

	bLoginInProgress = false;
	//bTriedPersistentAuth = false;

	UE_LOG(LogCrazyRoo, Warning, TEXT("[Session][CLIENT] 로그인 완료: %s %s"),
		bWasSuccessful ? TEXT("성공") : TEXT("실패"), *Error);

	OnLoginComplete.Broadcast(bWasSuccessful, Error);
}


// ─────────────────────────────────────────────────────────────
// [Server] 세션 생성 (데디서버 부팅 시 1회)
// ─────────────────────────────────────────────────────────────
void UCrazyRooSessionSubsystem::CreateDedicatedSession()
{
	//IOnlineSessionPtr Sessions = GetSessionInterface();
	//if (!Sessions.IsValid())
	//{
	//	return;
	//}
//
	//FOnlineSessionSettings Settings;
	//Settings.bIsDedicated          = true;   // 데디서버라고 명시
	//Settings.NumPublicConnections  = MaxPlayersPerRoom; //최대 4명
	//Settings.bShouldAdvertise      = true;   // 유저들이 새로고침 했을 때 내 방이 보이게 허용
	//Settings.bAllowJoinInProgress  = true;   // 로비 단계이므로 접속 난입 허용 (인게임 진입 시 막아야 함)
	// 데디서버는 Presence 기능을 쓸 수 없음
	// 반드시 false로 해야 유저들이 이 방을 검색할 수 있습니다.
	//Settings.bUsesPresence         = false;
	//Settings.bUseLobbiesIfAvailable = false; // 스팀 Lobbies API 안 씀 (서버 브라우저 방식 사용)
	//Settings.bAllowInvites         = false;  // [EOS] 초대 기능 미사용 (presence 없는 데디 세션과 세트)
	// 현재 플러그인이 Null(기본)이면 LAN 모드로 세팅합니다.
	//Settings.bIsLANMatch = (IOnlineSubsystem::Get()->GetSubsystemName() == NULL_SUBSYSTEM);

	// 방 목록 UI에 띄워줄 방 제목을 세션 데이터에 끼워 넣습니다.
	// 방생성하겠냐고 확인용 팝업창을 띄울지? , 일단 "CrazyRoo Room"으로 생성되게 고정
	//Settings.Set(RoomNameKey, FString(TEXT("CrazyRoo Room")), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	// 부팅 직후는 빈 방이므로 인원수 0으로  시작.
	// (클라 숨김 필터가 0명을 걸러내므로, 아무도 안 들어온 서버는 목록에 안 보임)
	//Settings.Set(PlayerCountKey, 0, EOnlineDataAdvertisementType::ViaOnlineService);

	//UE_LOG(LogCrazyRoo, Warning, TEXT("[Session][SERVER] CreateSession 요청 (LAN=%d)"), Settings.bIsLANMatch ? 1 : 0);
	// 엔진에게 세션 생성을 지시합니다. 완료 시 HandleCreateSessionComplete가 자동 호출됩니다.
	//Sessions->CreateSession(0, NAME_GameSession, Settings);

	// 부팅 시 최초 생성 — 빈 방(0명)으로 시작
	CreateDedicatedSessionInternal(0);
}

void UCrazyRooSessionSubsystem::CreateDedicatedSessionInternal(int32 AdvertisedCount)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		return;
	}

	LastAppliedCount = AdvertisedCount;
	PendingAdvertisedCount = AdvertisedCount;

	FOnlineSessionSettings Settings;
	Settings.bIsDedicated          = true;   // 데디서버라고 명시
	Settings.NumPublicConnections  = MaxPlayersPerRoom; //최대 4명
	Settings.bShouldAdvertise      = true;   // 유저들이 새로고침 했을 때 내 방이 보이게 허용
	Settings.bAllowJoinInProgress  = true;   // 로비 단계이므로 접속 난입 허용 (인게임 진입 시 막아야 함)
	// 데디서버는 Presence 기능을 쓸 수 없음
	// 반드시 false로 해야 유저들이 이 방을 검색할 수 있습니다.
	Settings.bUsesPresence         = false;
	Settings.bUseLobbiesIfAvailable = false; // 스팀 Lobbies API 안 씀 (서버 브라우저 방식 사용)
	Settings.bAllowInvites         = false;  // [EOS] 초대 기능 미사용 (presence 없는 데디 세션과 세트)
	// 현재 플러그인이 Null(기본)이면 LAN 모드로 세팅합니다.
	Settings.bIsLANMatch = (IOnlineSubsystem::Get()->GetSubsystemName() == NULL_SUBSYSTEM);

	// 방 목록 UI에 띄워줄 방 제목을 세션 데이터에 끼워 넣습니다.
	// 방생성하겠냐고 확인용 팝업창을 띄울지? , 일단 "CrazyRoo Room"으로 생성되게 고정
	Settings.Set(RoomNameKey, FString(TEXT("CrazyRoo Room")), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	// 현재 인원수. 0이면 클라 숨김 필터가 걸러내서 "아직 생성 안 된 방"으로 취급됨
	Settings.Set(PlayerCountKey, AdvertisedCount, EOnlineDataAdvertisementType::ViaOnlineService);

	UE_LOG(LogCrazyRoo, Warning, TEXT("[Session][SERVER] CreateSession 요청 (LAN=%d, 인원=%d)"),
		Settings.bIsLANMatch ? 1 : 0, AdvertisedCount);
	// 엔진에게 세션 생성을 지시합니다. 완료 시 HandleCreateSessionComplete가 자동 호출됩니다.
	Sessions->CreateSession(0, NAME_GameSession, Settings);

}

void UCrazyRooSessionSubsystem::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogCrazyRoo, Warning, TEXT("[Session] CreateSession 완료: %s (%s)"),
	*SessionName.ToString(), bWasSuccessful ? TEXT("성공") : TEXT("실패"));
	// 데디서버는 이미 LobbyMap을 로드한 상태로 켜졌기 때문에 맵 이동(ServerTravel)을 할 필요가 없습니다.
	// 이대로 유저를 기다립니다.

	// [워크어라운드] 재생성이 도는 사이(파괴~생성 약 1초)에 인원이 또 바뀌었으면
	// (예: 두 명이 연달아 입장) 최신 인원수로 한 번 더 재생성해서 따라잡는다.
	if (bWasSuccessful && PendingAdvertisedCount != LastAppliedCount)
	{
		UE_LOG(LogCrazyRoo, Warning, TEXT("[Session][SERVER] 재생성 중 인원 변동 감지 (%d→%d) — 재갱신"),
			LastAppliedCount, PendingAdvertisedCount);
		UpdateAdvertisedPlayerCount(PendingAdvertisedCount);
	}

}

// ─────────────────────────────────────────────────────────────
// [Server] 인원수 갱신 — GameMode의 PostLogin/Logout에서 호출
// [워크어라운드] 내장 EOS 플러그인의 UpdateSession이 속성 갱신을 백엔드에
// 반영하지 못하므로, 세션을 파괴하고 새 인원수로 재생성하는 방식 사용
// ─────────────────────────────────────────────────────────────


void UCrazyRooSessionSubsystem::UpdateAdvertisedPlayerCount(int32 Count)
{
	// [워크 어라운드]항상 최신 목표값을 기록 (재생성 중이어도 갱신 — 완료 시 이 값으로 따라잡음)
	PendingAdvertisedCount = Count;
	//

	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		return;
	}

	// [워크어라운드]
	if (bRecreateInProgress)
	{
		// 이미 파괴→생성이 도는 중 — PendingAdvertisedCount만 갱신해뒀으니
		// HandleCreateSessionComplete가 완료 시점에 최신값으로 다시 불러줌
		return;
	}
	//

	// 서버가 부팅 시 만들어둔 세션을 찾는다 (없으면 아직 생성 전이거나 실패한 상태)
	FNamedOnlineSession* Named = Sessions->GetNamedSession(NAME_GameSession);
	if (!Named)
	{
		UE_LOG(LogCrazyRoo, Warning, TEXT("[Session][SERVER] 인원수 갱신 실패 — 세션 없음 (Count=%d)"), Count);
		return;
	}

	// 기존 세팅(방 이름 등)을 복사한 뒤 인원수 키만 갱신해서 백엔드에 업데이트
	//FOnlineSessionSettings NewSettings = Named->SessionSettings;
	//NewSettings.Set(PlayerCountKey, Count, EOnlineDataAdvertisementType::ViaOnlineService);
	//Sessions->UpdateSession(NAME_GameSession, NewSettings, true); // true = 온라인 서비스(백엔드)까지 반영
	//UE_LOG(LogCrazyRoo, Warning, TEXT("[Session][SERVER] 인원수 광고 갱신: %d/%d"), Count, MaxPlayersPerRoom);

	// [워크 어라운드] 파괴 → 완료 콜백(HandleDestroyForRecreate) → 최신 인원수로 재생성
	bRecreateInProgress = true;
	RecreateDestroyHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &UCrazyRooSessionSubsystem::HandleDestroyForRecreate));

	UE_LOG(LogCrazyRoo, Warning, TEXT("[Session][SERVER] 인원수 갱신(%d/%d) — 세션 재생성 시작 (파괴 요청)"),
		Count, MaxPlayersPerRoom);
	Sessions->DestroySession(NAME_GameSession);
}

//[워크 어라운드]
void UCrazyRooSessionSubsystem::HandleDestroyForRecreate(FName SessionName, bool bWasSuccessful)
{
	if (IOnlineSessionPtr Sessions = GetSessionInterface())
	{
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(RecreateDestroyHandle);
	}
	bRecreateInProgress = false;

	UE_LOG(LogCrazyRoo, Warning, TEXT("[Session][SERVER] 재생성 — 파괴 완료(%s), 인원 %d로 새 세션 생성"),
		bWasSuccessful ? TEXT("성공") : TEXT("실패했지만 진행"), PendingAdvertisedCount);

	// 파괴 실패여도 생성 시도 (생성이 겹치면 CreateSession이 알아서 실패 로그를 남김)
	CreateDedicatedSessionInternal(PendingAdvertisedCount);
}


// ─────────────────────────────────────────────────────────────
// [Client] 방 검색 ("방 참가", "새로고침" 버튼)
// ─────────────────────────────────────────────────────────────

void UCrazyRooSessionSubsystem::FindRooms()
{
	//  이미 검색 중이면 새로 시작하지 않는다.
	//  CreateRoom이 켜둔 bAutoJoinIfEmpty는 지금 돌고 있는 검색이 끝날 때 그대로 판정된다.
	if (bFindInProgress)
	{
		UE_LOG(LogCrazyRoo, Warning, TEXT("[Session][CLIENT] 검색 진행 중 — 현재 검색 결과에 편승"));
		return;
	}

	// [EOS] 미로그인 상태에서는 FindSessions가 백엔드에서 거부됨 → 미리 걸러서 명확한 실패 처리
	if (!IsLoggedIn())
	{
		UE_LOG(LogCrazyRoo, Error, TEXT("[Session][CLIENT] EOS 미로그인 — FindRooms 불가. LoginToEOS를 먼저 호출하세요."));
		// CreateRoom 흐름으로 들어온 경우 플래그 정리 + 알맞은 델리게이트로 통지
		if (bAutoJoinIfEmpty)
		{
			bAutoJoinIfEmpty = false;
			OnCreateRoomFailed.Broadcast(TEXT("로그인이 필요합니다. 잠시 후 다시 시도해 주세요."));
		}
		else
		{
			OnFindRoomsComplete.Broadcast({}, false);
		}
		return;
	}

	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		// 네트워크 모듈이 꺼져있으면 즉시 실패 처리
		OnFindRoomsComplete.Broadcast({}, false);
		return;
	}

	// 검색 조건을 설정합니다
	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	SessionSearch->MaxSearchResults = 20;
	SessionSearch->bIsLanQuery = (IOnlineSubsystem::Get()->GetSubsystemName() == NULL_SUBSYSTEM);

	// [EOS] 온라인 검색일 때는 우리 게임 세션만 잡히도록 커스텀 키로 필터.
	// (EOS는 조건 없는 빈 쿼리 검색이 실패하는 경우가 있어 키 하나를 거는 게 안전.
	//  LAN 검색은 브로드캐스트 방식이라 쿼리 필터가 불필요하므로 온라인일 때만 설정)
	if (!SessionSearch->bIsLanQuery)
	{
		SessionSearch->QuerySettings.Set(RoomNameKey, FString(TEXT("CrazyRoo Room")), EOnlineComparisonOp::Equals);
	}


	// 데디서버를 찾는 것이므로 bUsesPresence 필터 조건을 달지 않습니다.
	UE_LOG(LogCrazyRoo, Warning, TEXT("[Session][CLIENT] FindSessions 시작 (LAN=%d)"), SessionSearch->bIsLanQuery ? 1 : 0);

	//    실제로 검색을 시작하기 직전에 플래그 ON
	//   (위의 Sessions 무효 조기 리턴 경로에서는 켜지면 안 되므로 반드시 이 위치)
	bFindInProgress = true;

	// 엔진에게 방 검색을 지시합니다. 완료 시 HandleFindSessionsComplete가 자동 호출
	Sessions->FindSessions(0, SessionSearch.ToSharedRef());
}

FCrazyRooRoomInfo UCrazyRooSessionSubsystem::MakeRoomInfo(int32 Index) const
{
	FCrazyRooRoomInfo Info;
	const FOnlineSessionSearchResult& R = SessionSearch->SearchResults[Index];

	// 원본 검색 데이터에서 필요한 정보만 뽑아 블루프린트용 구조체 만듬
	Info.SearchResultIndex = Index;
	Info.MaxPlayers        = R.Session.SessionSettings.NumPublicConnections;

	// 현재 인원수 계산:
	// 1순위 — 서버가 직접 광고한 CURPLAYERS 키 (EOS에서 신뢰 가능한 유일한 값)
	// 2순위 — 열린 슬롯 역산 (Null/LAN 폴백. EOS에선 NumOpenPublicConnections가 0으로 와서 4/4 버그의 원인이었음)

	// 클로드버전
	//int32 AdvertisedCount = -1;
	//if (R.Session.SessionSettings.Get(PlayerCountKey, AdvertisedCount) && AdvertisedCount >= 0)
	//{
	//	Info.CurrentPlayers = AdvertisedCount;
	//}
	//else
	//{
	//	Info.CurrentPlayers = Info.MaxPlayers - R.Session.NumOpenPublicConnections;
	//}

	int64 AdvertisedCount = -1;

	// SessionSettings 내부에 실제 세팅값들이 저장된 Settings TMap에서 키를 직접 찾습니다.
	if (const FOnlineSessionSetting* Setting = R.Session.SessionSettings.Settings.Find(PlayerCountKey))
	{
		// FVariantData가 들고 있는 값을 문자열로 변환한 뒤 int32로 안전하게 파싱합니다.
		// int64 링크 에러(LNK2019)를 피하고 타입 캐스팅 문제도 동시에 해결합니다.
		FString ValueStr = Setting->Data.ToString();
		AdvertisedCount = FCString::Atoi(*ValueStr);
	}

	if (AdvertisedCount >= 0)
	{
		Info.CurrentPlayers = AdvertisedCount;
	}
	else
	{
		Info.CurrentPlayers = Info.MaxPlayers - R.Session.NumOpenPublicConnections;
	}

	UE_LOG(LogCrazyRoo, Warning, TEXT("[Session][CLIENT] 방%d: CURPLAYERS읽기=%s, 값=%d, OpenConn=%d → 표시인원=%d"),
		Index,
		(AdvertisedCount >= 0) ? TEXT("성공") : TEXT("실패"),
		AdvertisedCount, R.Session.NumOpenPublicConnections, Info.CurrentPlayers);

	Info.PingMs = R.PingInMs;

	// 아까 서버가 세팅해둔 방 제목 데이터를 꺼내옴 (지금은 고정임)
	if (!R.Session.SessionSettings.Get(RoomNameKey, Info.RoomName))
	{
		Info.RoomName = TEXT("CrazyRoo Room");
	}
	return Info;
}

void UCrazyRooSessionSubsystem::HandleFindSessionsComplete(bool bWasSuccessful)
{
	// 검색이 끝났으므로 플래그 OFF (성공/실패 무관하게 항상)
	bFindInProgress = false;

	TArray<FCrazyRooRoomInfo> Rooms;
	if (bWasSuccessful && SessionSearch.IsValid())
	{
		// 검색된 방 갯수만큼 루프를 돌며 블루프린트용 데이터 구조체 배열을 만듬
		for (int32 i = 0; i < SessionSearch->SearchResults.Num(); ++i)
		{
			if (SessionSearch->SearchResults[i].IsValid())
			{
				Rooms.Add(MakeRoomInfo(i));
			}
		}
	}

	UE_LOG(LogCrazyRoo, Warning, TEXT("[Session][CLIENT] FindSessions 완료: %d개 발견"), Rooms.Num());

	// "방 생성" 흐름이면: 검색 결과를 보고 자동 판정
	if (bAutoJoinIfEmpty)
	{
		bAutoJoinIfEmpty = false;

		if (Rooms.Num() == 0)
		{
			// 검색된 방(데디서버)이 아예 없음 = 서버 프로그램이 안 켜져있음
			OnCreateRoomFailed.Broadcast(TEXT("게임 서버를 찾을 수 없습니다. (서버 꺼짐 또는 네트워크 문제)"));
			return;
		}
		if (Rooms[0].CurrentPlayers > 0)
		{
			// 서버는 켜져있는데 누군가 이미 안에 들어가 있음 = 내가 방장을 차지할 수 없음
			OnCreateRoomFailed.Broadcast(TEXT("이미 방이 존재합니다. '방 참가'를 이용해 주세요."));
			return;
		}
		// 비어있는 데디서버를 찾음
		// 처음으로 자동 입장합니다. 게임 규칙 상 첫 번째 입장자가 방장(Host)이 됩니다.
		JoinRoomByIndex(Rooms[0].SearchResultIndex);
		return;
	}
	// 방 참가 버튼을 눌러서 온 경우: UI 쪽으로 방 목록 데이터를 보여줌
	// [수정] 빈 서버(0명)는 "아직 생성되지 않은 방"으로 취급해 목록에서 숨긴다.
	//  → 처음엔 목록이 비어 보이고, 누군가 [방 생성]으로 입장한 뒤부터 방이 나타남.
	//  (위의 bAutoJoinIfEmpty 방 생성 분기는 빈 방을 찾아야 하므로 원본 Rooms를 그대로 사용)
	TArray<FCrazyRooRoomInfo> VisibleRooms;
	for (const FCrazyRooRoomInfo& Room : Rooms)
	{
		if (Room.CurrentPlayers > 0)
		{
			VisibleRooms.Add(Room);
		}
	}
	OnFindRoomsComplete.Broadcast(VisibleRooms, bWasSuccessful);
}

// ─────────────────────────────────────────────────────────────
// [Client] 방 생성 버튼 로직
// ─────────────────────────────────────────────────────────────

void UCrazyRooSessionSubsystem::CreateRoom()
{
	if (bFindInProgress)
	{
		OnCreateRoomFailed.Broadcast(TEXT("방 목록을 검색하는 중입니다. 잠시 후 다시 시도해 주세요."));
		return;
	}

	// 실제 방(세션)을 만들지 않고, 빈 방에 처음으로 들어가기 위해 플래그만 켜고 검색을 돌립니다.
	bAutoJoinIfEmpty = true;
	FindRooms();
}

// ─────────────────────────────────────────────────────────────
// [Client] 방 참가
// ─────────────────────────────────────────────────────────────

void UCrazyRooSessionSubsystem::JoinRoomByIndex(int32 SearchResultIndex)
{
	//   검색이 돌고 있는 동안은 참가 불가.
	//   진행 중인 검색이 SessionSearch를 새로 만들어 이 인덱스가 다른/없는 방을 가리킬 수 있다.
	if (bFindInProgress)
	{
		OnJoinRoomComplete.Broadcast(false, TEXT("방 목록을 검색하는 중입니다. 잠시 후 다시 시도해 주세요."));
		return;
	}

	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid() || !SessionSearch.IsValid()
		|| !SessionSearch->SearchResults.IsValidIndex(SearchResultIndex))
	{
		OnJoinRoomComplete.Broadcast(false, TEXT("잘못된 방 인덱스입니다. 목록을 새로고침 해주세요."));
		return;
	}

	// 이전에 참가했던 세션이 남아있으면 정리 (재접속 케이스)
	if (Sessions->GetNamedSession(NAME_GameSession))
	{
		Sessions->DestroySession(NAME_GameSession);
	}

	UE_LOG(LogCrazyRoo, Warning, TEXT("[Session][CLIENT] JoinSession 요청 (Index=%d)"), SearchResultIndex);
	// 엔진에게 진짜 서버 접속 요청을 보냅니다. 완료 시 HandleJoinSessionComplete가 자동 호출됩니다.
	Sessions->JoinSession(0, NAME_GameSession, SessionSearch->SearchResults[SearchResultIndex]);
}


void UCrazyRooSessionSubsystem::HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	// 서버 프로세스에는 로컬 플레이어가 없으므로 클라에서만 의미 있음
	if (IsRunningDedicatedServer())
	{
		return;
	}

	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		// 접속 실패 사유를 UI에 던져서 알림창을 띄우게 합니다.
		FString Reason;
		switch (Result)
		{
		case EOnJoinSessionCompleteResult::SessionIsFull:     Reason = TEXT("방이 가득 찼습니다. (4/4)"); break;
		case EOnJoinSessionCompleteResult::SessionDoesNotExist: Reason = TEXT("방이 더 이상 존재하지 않습니다."); break;
		default:                                               Reason = TEXT("알 수 없는 오류로 참가에 실패했습니다."); break;
		}
		OnJoinRoomComplete.Broadcast(false, Reason);
		return;
	}

	// 참가 성공 → 접속 문자열("IP:Port")을 얻어 실제 서버로 트래블
	IOnlineSessionPtr Sessions = GetSessionInterface();
	FString ConnectString;
	if (Sessions.IsValid() && Sessions->GetResolvedConnectString(SessionName, ConnectString))
	{
		UE_LOG(LogCrazyRoo, Warning, TEXT("[Session][CLIENT] Join 성공 → ClientTravel: %s"), *ConnectString);
		if (APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController())
		{
			OnJoinRoomComplete.Broadcast(true, FString());
			PC->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
			return;
		}
	}
	// IP 주소를 긁어오는 데 실패했을 때의 예외 처리
	OnJoinRoomComplete.Broadcast(false, TEXT("접속 주소를 확인할 수 없습니다."));
}



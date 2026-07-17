// Copyright Epic Games, Inc. All Rights Reserved.

#include "System/CrazyRooGameMode.h"
#include "Audio/CrazyRooAudioNetManager.h"
#include "Types/CrazyRooTypes.h"
#include "CrazyRoo.h"
#include "System/CrazyRooGameState.h"
#include "Player/CrazyRooPlayerState.h"
#include "Player/CrazyRooPlayerController.h"
#include "GameFramework/PlayerController.h"
#include "System/CrazyRooGameInstance.h"
#include "Manager/SharedCharacterManager.h"
#include "TimerManager.h"
#include "Manager/CrazyRooSessionSubsystem.h"

namespace
{
FString GetGameModeBGMTypeName(ECrazyRooBGMType BGMType)
{
	if (const UEnum* Enum = StaticEnum<ECrazyRooBGMType>())
	{
		return Enum->GetNameStringByValue(static_cast<int64>(BGMType));
	}

	return TEXT("Unknown");
}

void RequestInGameBGM(UWorld* World)
{
	UE_LOG(LogTemp, Log, TEXT("[CrazyRooGameMode] Request InGame BGM"));
	if (ACrazyRooAudioNetManager* AudioNetManager = ACrazyRooAudioNetManager::FindAudioNetManager(World))
	{
		AudioNetManager->RequestPlayBGM(ECrazyRooBGMType::InGame);
	}
}

void RequestStageBGM(UWorld* World, ECrazyRooBGMType BGMType)
{
	if (BGMType == ECrazyRooBGMType::None)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[CrazyRooGameMode] Request Stage BGM: %s"), *GetGameModeBGMTypeName(BGMType));
	if (ACrazyRooAudioNetManager* AudioNetManager = ACrazyRooAudioNetManager::FindAudioNetManager(World))
	{
		AudioNetManager->RequestPlayBGM(BGMType);
	}
}

void ScheduleInGameBGM(ACrazyRooGameMode* GameMode)
{
	if (!GameMode || !GameMode->GetWorld())
	{
		return;
	}

	FTimerHandle InGameBGMTimerHandle;
	GameMode->GetWorldTimerManager().SetTimer(
		InGameBGMTimerHandle,
		[GameMode]()
		{
			RequestInGameBGM(GameMode->GetWorld());
		},
		0.1f,
		false
	);
}

void ScheduleStageBGM(ACrazyRooGameMode* GameMode, ECrazyRooBGMType BGMType)
{
	if (!GameMode || !GameMode->GetWorld() || BGMType == ECrazyRooBGMType::None)
	{
		return;
	}

	FTimerHandle StageBGMTimerHandle;
	GameMode->GetWorldTimerManager().SetTimer(
		StageBGMTimerHandle,
		[GameMode, BGMType]()
		{
			RequestStageBGM(GameMode->GetWorld(), BGMType);
		},
		0.1f,
		false
	);
}
}

ACrazyRooGameMode::ACrazyRooGameMode()
{
	// stub
	GameStateClass        = ACrazyRooGameState::StaticClass();
	PlayerStateClass      = ACrazyRooPlayerState::StaticClass();
	PlayerControllerClass = ACrazyRooPlayerController::StaticClass();
	bUseSeamlessTravel    = true;
	DefaultPawnClass = nullptr;
}

void ACrazyRooGameMode::BeginPlay()
{
	Super::BeginPlay();
	FCrazyRooDebug::Print(this, TEXT("GameMode BeginPlay (서버 시작됨)"), FColor::Cyan);

	//   게임플레이 맵일 때만 공유 캐릭터 매니저 스폰
	//   ChangeMap()이 ServerTravel 전에 GI->CurrentMapId를 세팅해두므로,
	//   새 맵의 BeginPlay 시점엔 이미 올바른 MapId가 있음
	if (UCrazyRooGameInstance* GI = GetGameInstance<UCrazyRooGameInstance>())
	{
		if (const FCrazyRooStageInfo* Row = GI->FindStageRow(GI->CurrentMapId))
		{
			if (Row->bIsGameplayStage)
			{
				SharedCharacterManager = GetWorld()->SpawnActor<ASharedCharacterManager>(
					ASharedCharacterManager::StaticClass(),
					FVector::ZeroVector, FRotator::ZeroRotator);

				FCrazyRooDebug::Print(this,
					SharedCharacterManager ? TEXT("SharedCharacterManager 스폰 완료") : TEXT("SharedCharacterManager 스폰 실패"),
					SharedCharacterManager ? FColor::Cyan : FColor::Red);

				// [한길] 0711 매니저 스폰이 성공했을 때만 카운트다운 시작.
				if (SharedCharacterManager)
				{
					// [한길] 0712
					// 시작 방식 선택을 GameMode에서만 처리하여 테스트 모드와 카운트다운 흐름이 서로 꼬이지 않게 합니다.
					HandleGameStartMode();
				}
			}
			else if (Row->BGMType != ECrazyRooBGMType::None)
			{
				ScheduleStageBGM(this, Row->BGMType);
			}
		}
	}

}

// 플레이어가 서버에 접속을 완료했을 때 실행됨
void ACrazyRooGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// 접속 순번 부여 + GameInstance에 백업 저장 (트래블 후 PlayerState 재생성 대비)
	if (ACrazyRooPlayerState* PS = NewPlayer ? NewPlayer->GetPlayerState<ACrazyRooPlayerState>() : nullptr)
	{
		UCrazyRooGameInstance* GI = GetGameInstance<UCrazyRooGameInstance>();
		const int32 Order = NextJoinOrder++;
		PS->SetJoinOrder(Order);

		// 첫 번째 접속자 = 방장 (방 생성 버튼으로 빈 서버에 처음 들어온 사람)
		PS->SetIsHost(Order == 1);

		if (GI)
		{
			GI->SavePlayerJoinOrder(PS->GetPlayerId(), Order);
		}

		// 누가 몇 번째로 들어왔는지 명확히 로그
		FCrazyRooDebug::Print(this, FString::Printf(TEXT("플레이어-%d 접속 완료"), Order), FColor::Green);
	}

	// 데디 서버 기준: 접속한 PC 는 모두 원격이므로 IsLocal 은 0 이어야 정상
	// 접속한 총 인원수와 현재 접속한 사람의 정보를 로그로 출력 (데디 서버 디버깅용)
	const int32 Num = GetNumPlayers();
	const FString Msg = FString::Printf(TEXT("PostLogin: 접속 인원 = %d (PC=%s, IsLocal=%d)"),
		Num,
		NewPlayer ? *NewPlayer->GetName() : TEXT("null"),
		(NewPlayer && NewPlayer->IsLocalController()) ? 1 : 0);

	FCrazyRooDebug::Print(this, Msg, FColor::Green);

	// 방 인원 변동을 세션 백엔드에 광고 (데디서버에서만 의미 있음)
	// 다른 클라의 방 목록/방 생성 판정이 이 값을 읽는다
	if (IsRunningDedicatedServer())
	{
		if (UCrazyRooSessionSubsystem* SS = GetGameInstance()->GetSubsystem<UCrazyRooSessionSubsystem>())
		{
			SS->UpdateAdvertisedPlayerCount(Num);
		}
	}

	ApplyMatchStateForCurrentMap();
	InitPlayerForCurrentMap(NewPlayer);
	//** Stand Alone MainLevel 테스트용 게임 맵으로 바로 열고 싶을 때 **
	/*
	if (UCrazyRooGameInstance* GI = GetGameInstance<UCrazyRooGameInstance>())
	{
		if (const FCrazyRooStageInfo* Row = GI->FindStageRow(GI->CurrentMapId))
		{
			if (Row->bIsGameplayStage)
			{
				// 카메라를 공유 캐릭터로
				if (ACrazyRooPlayerController* PC = Cast<ACrazyRooPlayerController>(NewPlayer))
				{
					PC->Client_SetupGameplayCamera();
				}
				// 로비를 안 거쳤으니 역할이 None → 빈 역할 자동 배정
				if (ACrazyRooPlayerState* PS = NewPlayer ? NewPlayer->GetPlayerState<ACrazyRooPlayerState>() : nullptr)
				{
					if (PS->GetPlayerRole() == ECrazyRooPlayerRole::None)
					{
						PS->SetPlayerRole(FindFreeRole());
					}
				}
			}
		}
	}*/
	//**

}

void ACrazyRooGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// 규칙!! 데이터 테이블 RowName == 맵 에셋 이름
	// 어떤 경로(트래블/직접실행/PIE/데디)로 열리든 실제 월드 이름에서 MapId를 역산한다
	if (UCrazyRooGameInstance* GI = GetGameInstance<UCrazyRooGameInstance>())
	{
		const FString WorldName = GetWorld()->GetMapName().Mid(GetWorld()->StreamingLevelsPrefix.Len());
		GI->CurrentMapId = FName(*WorldName);

		if (!GI->FindStageRow(GI->CurrentMapId))
		{
			UE_LOG(LogCrazyRoo, Warning,
				TEXT("[GameMode] 맵 '%s' 에 해당하는 데이터 테이블 행 없음 — RowName을 맵 이름과 맞춰주세요"),
				*WorldName);
		}
	}
}

// 플레이어가 방을 나갔을 때 실행됨
void ACrazyRooGameMode::Logout(AController* Exiting)
{
	// 방장 승계: 나가는 사람이 방장이면 남은 인원 중 JoinOrder 최소자에게
	if (ACrazyRooPlayerState* ExitingPS = Exiting ? Exiting->GetPlayerState<ACrazyRooPlayerState>() : nullptr)
	{
		if (ExitingPS->IsHost())
		{
			ACrazyRooPlayerState* NewHost = nullptr;
			if (const AGameStateBase* GS = GetGameState<AGameStateBase>())
			{
				for (APlayerState* Raw : GS->PlayerArray)
				{
					ACrazyRooPlayerState* PS = Cast<ACrazyRooPlayerState>(Raw);
					if (!PS || PS == ExitingPS)
					{
						continue; // 나가는 본인은 제외
					}
					if (!NewHost || PS->GetJoinOrder() < NewHost->GetJoinOrder())
					{
						NewHost = PS;
					}
				}
			}

			if (NewHost)
			{
				NewHost->SetIsHost(true);
				FCrazyRooDebug::Print(this, FString::Printf(
					TEXT("방장 승계 → 플레이어-%d"), NewHost->GetJoinOrder()), FColor::Cyan);
			}
		}
	}
	// 고민

	// 누가 나갔는지 로그로 출력
	const FString Msg = FString::Printf(TEXT("Logout: %s 나감"),Exiting ? *Exiting->GetName() : TEXT("null"));
	FCrazyRooDebug::Print(this, Msg, FColor::Orange);

	// 방이 완전히 비면 순번을 리셋 → 다음에 [방 생성]으로 들어오는 사람이 다시 1번(방장)이 됨
	// (이 시점 PlayerArray에는 나가는 본인이 아직 남아있을 수 있으므로 1명 이하 = 사실상 빈 방)
	if (const AGameStateBase* GS = GetGameState<AGameStateBase>())
	{
		if (GS->PlayerArray.Num() <= 1)
		{
			NextJoinOrder = 1;
			FCrazyRooDebug::Print(this, TEXT("방이 비었음 — JoinOrder 리셋 (다음 입장자 = 방장)"), FColor::Cyan);
		}
	}

	Super::Logout(Exiting);

	// 인원 변동 갱신 — 나간 사람이 PlayerArray에서 빠진 뒤 세야 하므로 다음 틱에 실행
	if (IsRunningDedicatedServer())
	{
		GetWorldTimerManager().SetTimerForNextTick([this]()
		{
			const AGameStateBase* GS = GetGameState<AGameStateBase>();
			const int32 Count = GS ? GS->PlayerArray.Num() : 0;
			if (UCrazyRooSessionSubsystem* SS = GetGameInstance()->GetSubsystem<UCrazyRooSessionSubsystem>())
			{
				SS->UpdateAdvertisedPlayerCount(Count);
			}
		});
	}

}

void ACrazyRooGameMode::HandleSeamlessTravelPlayer(AController*& C)
{
	Super::HandleSeamlessTravelPlayer(C);

	ACrazyRooPlayerState* PS = C ? C->GetPlayerState<ACrazyRooPlayerState>() : nullptr;
	UCrazyRooGameInstance* GI = GetGameInstance<UCrazyRooGameInstance>();
	ACrazyRooPlayerController* PC = Cast<ACrazyRooPlayerController>(C);

	// 맵 이동 직후 플레이어가 새로 생성된 PlayerState을 들고오기 때문에 GameInstance에 저장되어 있는 ID따라 배정해줘야함
	if (PS && GI && PC)
	{
		// 접속 순번 복원(언제든 삭제 가능)
		const int32 SavedOrder = GI->GetSavedPlayerJoinOrder(PS->GetPlayerId());
		if (SavedOrder > 0)
		{
			PS->SetJoinOrder(SavedOrder);
			PS->SetIsHost(SavedOrder == 1);   // 방장 복원 — 트래블마다 PlayerState가 새로 생겨 리셋되므로
		}

		// [기존 코드] 역할 복원
		const ECrazyRooPlayerRole Saved = GI->GetSavedPlayerRole(PS->GetPlayerId());
		if (Saved != ECrazyRooPlayerRole::None)
		{
			PS->SetPlayerRole(Saved);
		}

		//테스트용 추가
		if (const FCrazyRooStageInfo* Row = GI->FindStageRow(GI->CurrentMapId))
		{
			if (Row->bIsGameplayStage)
			{
				PC->Client_SetupGameplayCamera();
			}
		}

	}

	ApplyMatchStateForCurrentMap();
	InitPlayerForCurrentMap(C);
}

// [Server] 클라이언트가 컨트롤러를 통해 역할을 요청했을 때 판정하는 함수
bool ACrazyRooGameMode::RequestRole(APlayerController* Player, ECrazyRooPlayerRole DesiredRole)
{
	ACrazyRooPlayerState* PS = Player ? Player->GetPlayerState<ACrazyRooPlayerState>() : nullptr;
	if (!PS)
	{
		return false;
	}

	// 이미 내가 그 역할이면 성공 처리
	if (PS->GetPlayerRole() == DesiredRole)
	{
		return true;
	}

	// 역할을 비우겠다(None)고 요청한 경우 처리
	if (DesiredRole == ECrazyRooPlayerRole::None)
	{
		PS->SetPlayerRole(ECrazyRooPlayerRole::None);

		//GameInstance에 플레이어 역할 저장
		if (UCrazyRooGameInstance* GI = GetGameInstance<UCrazyRooGameInstance>())
		{
			GI->SavePlayerRole(PS->GetPlayerId(), ECrazyRooPlayerRole::None);
		}
		return true;
	}

	// 다른 사람이 이미 역할을 배정해 사용하고 있는지 검사
	if (IsRoleTaken(DesiredRole))
	{
		// 접근시 로그를 띄우고 false(거부) 반환
		const FString Msg = FString::Printf(TEXT("[SERVER] 플레이어-%d 요청 거부: %s 는 이미 사용중"),
			PS->GetJoinOrder(), CrazyRooRoleToString(DesiredRole));
		FCrazyRooDebug::Print(this, Msg, FColor::Red);
		return false;
	}

	// -------------------------------------------------------------
	// 정상적으로 역할을 승인하는 경우 처리
	// -------------------------------------------------------------

	//GameInstance에 플레이어 역할 저장
	PS->SetPlayerRole(DesiredRole);
	if (UCrazyRooGameInstance* GI = GetGameInstance<UCrazyRooGameInstance>())
	{
		GI->SavePlayerRole(PS->GetPlayerId(), DesiredRole);
	}

	//디버그 메시지
	const FString Msg = FString::Printf(TEXT("[SERVER] 플레이어-%d 역할 승인 -> %s"),
		PS->GetJoinOrder(), CrazyRooRoleToString(DesiredRole));
	FCrazyRooDebug::Print(this, Msg, FColor::Green);

	return true;
}

void ACrazyRooGameMode::ChangeMap(FName MapId)
{
	UWorld* World = GetWorld();
	UCrazyRooGameInstance* GI = GetGameInstance<UCrazyRooGameInstance>();
	if (!World || !GI)
	{
		return;
	}

	const FString Path = GI->GetStageMapPath(MapId);
	if (Path.IsEmpty())
	{
		FCrazyRooDebug::Print(this, FString::Printf(TEXT("거부: 맵 '%s' 경로를 찾을 수 없음"), *MapId.ToString()), FColor::Red);
		return;
	}

	// 도착 후 MatchState 결정에 쓸 MapId 저장
	GI->CurrentMapId = MapId;

	// 레벨 이동 시 로그
	FCrazyRooDebug::Print(this, FString::Printf(TEXT("맵 '%s' 로 이동: %s"), *MapId.ToString(), *Path), FColor::Cyan);

	World->ServerTravel( Path);

}

ASharedCharacterManager* ACrazyRooGameMode::GetSharedCharacterManager() const
{
	return SharedCharacterManager;
}

void ACrazyRooGameMode::InitPlayerForCurrentMap(AController* C)
{
	UCrazyRooGameInstance* GI = GetGameInstance<UCrazyRooGameInstance>();
	const FCrazyRooStageInfo* Row = GI ? GI->FindStageRow(GI->CurrentMapId) : nullptr;

	if (!Row || !Row->bIsGameplayStage)
	{
		return; // 로비 등 비게임 맵이면 스킵
	}

	// 카메라를 공유 캐릭터로
	if (ACrazyRooPlayerController* PC = Cast<ACrazyRooPlayerController>(C))
	{
		PC->Client_SetupGameplayCamera();
	}

	// 역할이 None인 채 게임 맵에 도착 → 빈 역할 자동 배정 (직접실행/난입 안전망)
	//if (ACrazyRooPlayerState* PS = C ? C->GetPlayerState<ACrazyRooPlayerState>() : nullptr)
	//{
	//	if (PS->GetPlayerRole() == ECrazyRooPlayerRole::None)
	//	{
	//		const ECrazyRooPlayerRole AutoRole = FindFreeRole();
	//		PS->SetPlayerRole(AutoRole);
	//		if (GI)
	//		{
	//			GI->SavePlayerRole(PS->GetPlayerId(), AutoRole); // 이후 트래블에도 유지
	//		}
	//	}
	//}
}

bool ACrazyRooGameMode::AreAllPlayersReady() const
{
	const AGameStateBase* GS = GetGameState<AGameStateBase>();
	if (!GS)
	{
		return false;
	}

	// 인원 검사 — MinPlayersToStart 미만이면 시작 불가
	if (GS->PlayerArray.Num() < MinPlayersToStart)
	{
		return false;
	}

	// 준비 검사 — 방장은 시작 버튼을 누르는 것 자체가 의사표시이므로 제외
	for (APlayerState* Raw : GS->PlayerArray)
	{
		const ACrazyRooPlayerState* PS = Cast<ACrazyRooPlayerState>(Raw);
		if (PS && !PS->IsHost() && !PS->IsReady())
		{
			return false;
		}
	}
	return true;
}

bool ACrazyRooGameMode::IsDevSoloStart() const
{
	return bDevAllowSoloStart;
}

// 빈 역할을 찾아주는 함수입니다.
ECrazyRooPlayerRole ACrazyRooGameMode::FindFreeRole() const
{
	// 역할을 검사할 순서를 미리 배열에 저장
	const ECrazyRooPlayerRole Order[] = {
		ECrazyRooPlayerRole::P_Left,
		ECrazyRooPlayerRole::P_Right,
		ECrazyRooPlayerRole::P_Shooter,
		ECrazyRooPlayerRole::P_Acrobat
	};


	for (ECrazyRooPlayerRole Candidate : Order)
	{
		// 어떤 플레이어가 역할을 할당받았는지 확인
		if (!IsRoleTaken(Candidate))
		{
			//비어있으면 그 역할 즉시 반환
			return Candidate;
		}
	}

	// 다 배정되어 있으면 None을 반환합니다
	return ECrazyRooPlayerRole::None;
}

// 다른 플레이어가  이미 그 역할을 선점했는지 PlayerArray(접속자 목록)를 돌면서 확인하는 함수
bool ACrazyRooGameMode::IsRoleTaken(ECrazyRooPlayerRole PlayerRole) const
{
	if (PlayerRole == ECrazyRooPlayerRole::None)
	{
		return false;
	}
	if (const AGameStateBase* GS = GetGameState<AGameStateBase>())
	{
		for (APlayerState* RawPS : GS->PlayerArray)
		{
			const ACrazyRooPlayerState* PS = Cast<ACrazyRooPlayerState>(RawPS);
			if (PS && PS->GetPlayerRole() == PlayerRole)
			{
				return true;
			}
		}
	}
	return false;
}

void ACrazyRooGameMode::ApplyMatchStateForCurrentMap()
{
	ACrazyRooGameState* GS = GetGameState<ACrazyRooGameState>();
	UCrazyRooGameInstance* GI = GetGameInstance<UCrazyRooGameInstance>();
	if (!GS || !GI)
	{
		return;
	}

	// 서버는 "지금 맵이 뭔지"만 GameState에 실어 보낸다.
	// 그 맵의 UI가 뭔지는 클라가 자기 데이터 테이블에서 직접 조회한다 (테이블은 서버·클라 둘 다 갖고 있음).
	if (!GI->FindStageRow(GI->CurrentMapId))
	{
		FCrazyRooDebug::Print(this, FString::Printf(
			TEXT("맵 '%s' 행 못찾음"), *GI->CurrentMapId.ToString()), FColor::Orange);
	}

	GS->SetCurrentMapId(GI->CurrentMapId);
}

// [한길] 0712
// GameStartMode에 따라 실게임 카운트다운 또는 테스트용 즉시 이동 모드를 시작합니다.
// Manager는 캐릭터에 이동 모드를 적용하는 역할만 하고, 시작 절차의 기준은 GameMode가 가집니다.
void ACrazyRooGameMode::HandleGameStartMode()
{
	if (!HasAuthority())
	{
		return;
	}

	if (!SharedCharacterManager)
	{
		UE_LOG(LogCrazyRoo, Error, TEXT("HandleGameStartMode failed: SharedCharacterManager is NULL"));
		return;
	}

	switch (GameStartMode)
	{
	case ECrazyRooGameStartMode::Normal:
		// 실게임 기본 흐름입니다. 먼저 정지 상태로 두고, 카운트다운이 끝나면 StartGame()에서 AutoMove로 전환합니다.
		// Normal에서는 기존처럼 역할별 입력 제한을 유지합니다.
		SharedCharacterManager->SetIgnorePlayerRole(false);
		SharedCharacterManager->StopMove();
		StartCountdown();
		UE_LOG(LogCrazyRoo, Warning, TEXT("Game Start Mode: Normal"));
		break;

	case ECrazyRooGameStartMode::ManualTest:
		// 수동 이동 테스트용 흐름입니다. 카운트다운을 생략하고 W/S DebugMoveForward 입력을 바로 받을 수 있게 합니다.
		GetWorldTimerManager().ClearTimer(CountdownTimer);
		// 테스트 모드에서는 역할 배정을 무시하여 한 플레이어가 모든 조작을 확인할 수 있게 합니다.
		SharedCharacterManager->SetIgnorePlayerRole(true);
		SharedCharacterManager->StartManualMove();
		ScheduleInGameBGM(this);
		UE_LOG(LogCrazyRoo, Warning, TEXT("Game Start Mode: ManualTest"));
		break;

	case ECrazyRooGameStartMode::AutoMoveTest:
		// 자동 이동만 빠르게 확인하는 테스트용 흐름입니다. 카운트다운 없이 즉시 AutoMove로 전환합니다.
		GetWorldTimerManager().ClearTimer(CountdownTimer);
		// 테스트 모드에서는 역할 배정을 무시하여 한 플레이어가 모든 조작을 확인할 수 있게 합니다.
		SharedCharacterManager->SetIgnorePlayerRole(true);
		SharedCharacterManager->StartAutoMove();
		ScheduleInGameBGM(this);
		UE_LOG(LogCrazyRoo, Warning, TEXT("Game Start Mode: AutoMoveTest"));
		break;

	default:
		UE_LOG(LogCrazyRoo, Error, TEXT("Unknown GameStartMode"));
		break;
	}
}

void ACrazyRooGameMode::StartCountdown()
{
	CurrentCountdown = CountdownSeconds;

	if (ACrazyRooGameState* GS = GetGameState<ACrazyRooGameState>())
	{
		GS->SetCountdown(CurrentCountdown);
	}

	GetWorldTimerManager().SetTimer(
		CountdownTimer,
		this,
		&ACrazyRooGameMode::CountdownTick,
		1.f,
		true
	);
}

void ACrazyRooGameMode::CountdownTick()
{
	CurrentCountdown--;

	if (ACrazyRooGameState* GS =
		GetGameState<ACrazyRooGameState>())
	{
		GS->SetCountdown(CurrentCountdown);
	}

	if (CurrentCountdown <= 0)
	{
		GetWorldTimerManager().ClearTimer(
			CountdownTimer);

		StartGame();
	}
}

void ACrazyRooGameMode::StartGame()
{
	if (!SharedCharacterManager)
	{
		return;
	}

	// [한길] 0712
	// Normal 모드의 카운트다운이 끝났을 때만 호출되는 실제 게임 시작 처리입니다.
	SharedCharacterManager->StartAutoMove();
	ScheduleInGameBGM(this);
}

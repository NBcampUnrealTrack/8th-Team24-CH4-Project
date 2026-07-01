// RailCoopGameMode.cpp

#include "LHG/GameMode/RailCoopGameMode.h"

#include "LHG/Manager/SharedCharacterManager.h"
#include "LHG/Character/SharedRailCharacter.h"
#include "LHG/Controller/RailCoopPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

ARailCoopGameMode::ARailCoopGameMode()
{
	// 이 GameMode에서 사용할 PlayerController 클래스 지정
	PlayerControllerClass = ARailCoopPlayerController::StaticClass();

	// 중요:
	// DefaultPawnClass는 비워둔다.
	// 플레이어마다 Pawn이 자동 스폰되는 것을 막기 위함.
	// 일단 지금은 레벨에 플레이어를 배치한 상태로 플레이하는것으로 상정함.
	DefaultPawnClass = nullptr;
}

void ARailCoopGameMode::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("GameMode BeginPlay"));

	// 서버에서만 공유 캐릭터 생성
	if (!HasAuthority())
	{
		return;
	}

	// Manager를 서버에서 하나만 스폰
	SharedCharacterManager = GetWorld()->SpawnActor<ASharedCharacterManager>(
			ASharedCharacterManager::StaticClass(),
			FVector::ZeroVector,
			FRotator::ZeroRotator
		);

	if (SharedCharacterManager)
	{
		UE_LOG(LogTemp,Warning,TEXT("SharedCharacterManager Spawned"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("SharedCharacterManager Spawn Failed"));
	}
}

void ARailCoopGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	ARailCoopPlayerController* RailPC =
		Cast<ARailCoopPlayerController>(NewPlayer);

	if (!RailPC)
	{
		return;
	}

	// 접속 순서에 따라 역할 부여
	ERailPlayerRole AssignedRole = GetRoleByJoinIndex(JoinedPlayerCount);

	RailPC->SetPlayerRole(AssignedRole);

	JoinedPlayerCount++;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Player Joined. Assigned Role Index: %d"),
		static_cast<int32>(AssignedRole)
	);

	JoinedPlayerCount++;
}

ASharedCharacterManager* ARailCoopGameMode::GetSharedCharacterManager() const
{
	return SharedCharacterManager;
}

ERailPlayerRole ARailCoopGameMode::GetRoleByJoinIndex(int32 Index) const
{
	switch (Index)
	{
	case 0:
		return ERailPlayerRole::LeftControl;

	case 1:
		return ERailPlayerRole::RightControl;

	case 2:
		return ERailPlayerRole::JumpSlideControl;

	case 3:
		return ERailPlayerRole::AimShootControl;

	default:
		// 4명 초과 접속 시 임시로 사격 역할
		return ERailPlayerRole::AimShootControl;
	}
}

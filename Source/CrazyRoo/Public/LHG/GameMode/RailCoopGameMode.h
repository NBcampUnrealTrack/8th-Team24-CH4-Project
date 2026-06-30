// RailCoopGameMode.h

#pragma once

#include "CoreMinimal.h"
#include "LHG/Types/RailTypes.h"
#include "GameFramework/GameModeBase.h"
#include "RailCoopGameMode.generated.h"

class ASharedRailCharacter;
class ARailCoopPlayerController;
class ASharedCharacterManager;

UCLASS()
class CRAZYROO_API ARailCoopGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARailCoopGameMode();

	virtual void BeginPlay() override;

	// 플레이어가 서버에 접속할 때 호출됨
	virtual void PostLogin(APlayerController* NewPlayer) override;

	// 공유 캐릭터 반환
	ASharedCharacterManager* GetSharedCharacterManager() const;


protected:
	// GameMode가 서버에서 Manager를 하나만 스폰한다
	UPROPERTY()
	ASharedCharacterManager* SharedCharacterManager = nullptr;

	// 접속한 플레이어 수
	int32 JoinedPlayerCount = 0;

	// 접속 순서에 따라 역할 배정
	ERailPlayerRole GetRoleByJoinIndex(int32 Index) const;
};

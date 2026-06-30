// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CrazyRooTypes.h"
#include "GameFramework/GameModeBase.h"
#include "CrazyRooGameMode.generated.h"

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class ACrazyRooGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	/** Constructor */
	ACrazyRooGameMode();
	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	// [Server] 역할 요청 판정. 성공 true / 이미 역할이 배정되어 있으면 false
	bool RequestRole(APlayerController* Player, ECrazyRooPlayerRole DesiredRole);

protected:

	// 다음 비어있는 역할 반환 (없으면 None)
	ECrazyRooPlayerRole FindFreeRole() const;

	// 특정 역할이 이미 배정됐는지
	bool IsRoleTaken(ECrazyRooPlayerRole PlayerRole) const;
};




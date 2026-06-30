// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CrazyRooGameMode.generated.h"


enum class ECrazyRooPlayerRole : uint8;

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
	virtual void HandleSeamlessTravelPlayer(AController*& C) override; // 심리스 트래블로 넘어온 플레이어를 처리하는 함수

	// [Server] 역할 요청 판정. 성공 true / 이미 역할이 배정되어 있으면 false
	bool RequestRole(APlayerController* Player, ECrazyRooPlayerRole DesiredRole);


	// [Server] MapId 에 해당하는 맵으로 모든 플레이어이동
	UFUNCTION(BlueprintCallable, Category = "CrazyRoo")
	void ChangeMap(FName MapId);

protected:

	// 다음 비어있는 역할 반환 (없으면 None)
	// P1->P2->P3->P4 순서대로 남는 자리를 찾아주는 함수입니다.
	ECrazyRooPlayerRole FindFreeRole() const;

	// 특정 역할이 이미 배정됐는지
	// 방에 있는 플레이어를 돌아 검사해 겹치는 역할이 있는지 검사합니다.
	bool IsRoleTaken(ECrazyRooPlayerRole PlayerRole) const;
};




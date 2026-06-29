// CrazyRoo 플레이어 스테이트 클래스
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "CrazyRooPlayerState.generated.h"

UCLASS()
class CRAZYROO_API ACrazyRooPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	// 기본 생성자
	ACrazyRooPlayerState();
public:
	virtual void BeginPlay() override;
};

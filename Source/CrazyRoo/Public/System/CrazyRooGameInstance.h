// CrazyRoo 게임 인스턴스 클래스
#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "CrazyRooGameInstance.generated.h"

UCLASS()
class CRAZYROO_API UCrazyRooGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// 기본 생성자
	UCrazyRooGameInstance();

	virtual void Init() override;
};

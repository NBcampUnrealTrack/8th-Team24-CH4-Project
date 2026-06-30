// SharedCharacterManager.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LHG/Types/RailTypes.h"
#include "SharedCharacterManager.generated.h"

class ASharedRailCharacter;

UCLASS()
class CRAZYROO_API ASharedCharacterManager :public AActor
{
	GENERATED_BODY()

public:
	ASharedCharacterManager();

	virtual void BeginPlay()override;

	// 서버에서 공유 캐릭터를 찾거나 저장한다
	void InitializeSharedCharacter();

	// PlayerController가 명령을 보낼 때 호출한다
	void RequestMoveLeft(ERailPlayerRole RequestRole);
	void RequestMoveRight(ERailPlayerRole RequestRole);
	void RequestJump(ERailPlayerRole RequestRole);
	void RequestSlide(ERailPlayerRole RequestRole);
	void RequestFireToLocation(ERailPlayerRole RequestRole, const FVector& TargetLocation);
	void RequestRotateCameraLeft(ERailPlayerRole RequestRole);
	void RequestRotateCameraRight(ERailPlayerRole RequestRole);
	void RequestFireAimRay(ERailPlayerRole RequestRole, const FVector& AimOrigin, const FVector& AimDirection);

protected:
	// 맵에 배치된 BP_SharedRailCharacter를 저장
	UPROPERTY()
	ASharedRailCharacter* SharedCharacter = nullptr;

public:
	// 테스트 목적으로 생성. true면 누구나 모든 입력 가능, false면 역할별 입력만 가능.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Debug")
	bool bIgnorePlayerRole = true;
};

// CrazyRoo 플레이어 스테이트 클래스
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "CrazyRooTypes.h"
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


	// 복제할 변수를 엔진에 등록 (이게 있어야 클라로 내려감)
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "CrazyRoo")
	ECrazyRooPlayerRole GetPlayerRole() const;

	// --- [Server Only] 역할 설정. GameMode 만 호출 ---
	void SetPlayerRole(ECrazyRooPlayerRole NewRole);

protected:
	// ReplicatedUsing → 클라에 값이 도착하면 OnRep_PlayerRole 자동 호출
	UPROPERTY(ReplicatedUsing = OnRep_PlayerRole, BlueprintReadOnly, Category = "CrazyRoo")
	ECrazyRooPlayerRole PlayerRole = ECrazyRooPlayerRole::None;

	UFUNCTION()
	void OnRep_PlayerRole();
};

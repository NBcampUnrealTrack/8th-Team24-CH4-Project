// CrazyRoo 플레이어 스테이트 클래스
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "CrazyRooPlayerState.generated.h"

enum class ECrazyRooPlayerRole : uint8;
UCLASS()
class CRAZYROO_API ACrazyRooPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	// 기본 생성자
	ACrazyRooPlayerState();
public:
	virtual void BeginPlay() override;

	//  복제할 변수(PlayerRole)를 엔진에 등록합니다. (이게 있어야 클라로 전송함)
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 다른 클래스나 블루프린트에서 내 역할을 반환해주는 함수
	UFUNCTION(BlueprintPure, Category = "CrazyRoo")
	ECrazyRooPlayerRole GetPlayerRole() const;

	// [Server Only] 역할 설정
	// 오직 서버의 GameMode만 이 함수를 통해 내 역할을 강제로 바꿀 수 있음
	void SetPlayerRole(ECrazyRooPlayerRole NewRole);

protected:
	// 내 역할 변수입니다.
	// ReplicatedUsing = OnRep_PlayerRole 의 의미: "서버가 이 값을 바꿔서 내 클라로 전송이 완료되는 순간, 즉시 OnRep_PlayerRole() 함수를 자동 실행해라" 라는 뜻
	UPROPERTY(ReplicatedUsing = OnRep_PlayerRole, BlueprintReadOnly, Category = "CrazyRoo")
	ECrazyRooPlayerRole PlayerRole;

	// 변경된 데이터가 클라이언트에 무사히 도착했을 때 실행되어 화면(UI)을 갱신하는 함수
	UFUNCTION()
	void OnRep_PlayerRole();
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Player/CrazyRooPlayerState.h"
#include "CrazyRooGameState.generated.h"

/**
 *
 */

enum class ECrazyRooStageState : uint8;


// UI 가 바인딩할 델리게이트 — MatchState 가 바뀌면 알림
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCrazyRooMapChanged, FName, NewMapId);
// UI 캐러셀 인덱스 동기화용 델리게이트 추가
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCrazyRooCarouselIndexChanged, int32, NewIndex);
// [한길] 0711 카운트다운 값이 바뀌었을 때 UI에 알려주는 이벤트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCountdownChanged, int32, NewCountdown);

UCLASS()
class CRAZYROO_API ACrazyRooGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ACrazyRooGameState();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 현재 맵 식별자 (데이터 테이블 조회 키, 클라에도 정확히 동기화됨)
	UFUNCTION(BlueprintPure, Category = "CrazyRoo")
	FName GetCurrentMapId() const;

	// [Server Only] GameMode가 트래블 완료 후 호출
	void SetCurrentMapId(FName NewMapId);

	UPROPERTY(BlueprintAssignable, Category = "CrazyRoo")
	FOnCrazyRooMapChanged OnCurrentMapChanged;

	// 블루프린트에서 바인딩할 캐러셀 이벤트
	UPROPERTY(BlueprintAssignable, Category = "CrazyRoo")
	FOnCrazyRooCarouselIndexChanged OnCarouselIndexChanged;

	// 서버가 캐러셀 인덱스를 바꿀 때 쓰는 함수
	void SetCarouselIndex(int32 NewIndex);

	// 로비 인원 변동(입장/퇴장) 시 UI가 구독할 이벤트
	UPROPERTY(BlueprintAssignable, Category = "CrazyRoo")
	FOnLobbyStateChanged OnLobbyPlayersChanged;

	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_CurrentMapId)
	FName CurrentMapId;

	UFUNCTION()
	void OnRep_CurrentMapId();

	// 리플리케이트 되는 맵 캐러셀 인덱스 변수
	UPROPERTY(ReplicatedUsing = OnRep_CarouselIndex)
	int32 CarouselIndex = 0;

	UFUNCTION()
	void OnRep_CarouselIndex();

	// [한길] 0711 카운트다운
	// 서버에서 변경한 뒤 모든 클라이언트에 복제되는 값
	UPROPERTY(ReplicatedUsing = OnRep_Countdown, BlueprintReadOnly, Category = "CrazyRoo|Countdown")
	int32 Countdown = 0;

	// 클라이언트가 새로운 Countdown을 복제받았을 때 호출
	UFUNCTION()
	void OnRep_Countdown();

public:
	// [한길] 0711 UI가 구독할 카운트다운 변경 이벤트
	UPROPERTY(BlueprintAssignable, Category = "CrazyRoo|Countdown")
	FOnCountdownChanged OnCountdownChanged;

	// 서버에서 카운트다운 값을 변경
	void SetCountdown(int32 NewCountdown);

	// 현재 복제된 카운트다운 값을 반환합니다.
	// UI가 처음 생성될 때 초기 숫자를 표시하기 위해 사용합니다.
	UFUNCTION(BlueprintPure, Category = "CrazyRoo|Countdown")
	int32 GetCountdown() const { return Countdown; }
};

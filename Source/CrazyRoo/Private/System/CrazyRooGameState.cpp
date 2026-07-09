	// Fill out your copyright notice in the Description page of Project Settings.


#include "System/CrazyRooGameState.h"
#include "CrazyRoo.h"
#include "Types/CrazyRooTypes.h"
#include "Net/UnrealNetwork.h"

ACrazyRooGameState::ACrazyRooGameState()
{
	CurrentMapId = NAME_None;
}

void ACrazyRooGameState::BeginPlay()
{
	Super::BeginPlay();


	FCrazyRooDebug::PrintSystem(this, TEXT("GameState BeginPlay"), FColor::White);
}

void ACrazyRooGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACrazyRooGameState, CurrentMapId);
	DOREPLIFETIME(ACrazyRooGameState, CarouselIndex);
}

FName ACrazyRooGameState::GetCurrentMapId() const
{
	return CurrentMapId;
}

void ACrazyRooGameState::SetCurrentMapId(FName NewMapId)
{
	if (!HasAuthority() || CurrentMapId == NewMapId)
	{
		return;
	}

	CurrentMapId = NewMapId;

	FCrazyRooDebug::Print(this, FString::Printf(
		TEXT("[SERVER] CurrentMapId -> %s"), *CurrentMapId.ToString()), FColor::White);

	// 서버 자신은 OnRep이 자동으로 안 불리므로 직접 브로드캐스트
	OnCurrentMapChanged.Broadcast(CurrentMapId);
}

void ACrazyRooGameState::OnRep_CurrentMapId()
{
	//레벨 넘어갈때 여기가 호출되면서 playerState가 없어서 ?가 나오는거 같음 호출 시기에
	FCrazyRooDebug::PrintSystem(this, FString::Printf(TEXT("OnRep_CurrentMapId = %s"), *CurrentMapId.ToString()), FColor::White);

	OnCurrentMapChanged.Broadcast(CurrentMapId);
}
void ACrazyRooGameState::SetCarouselIndex(int32 NewIndex)
{
	if (HasAuthority() && CarouselIndex != NewIndex)
	{
		CarouselIndex = NewIndex;
		// 서버 자신도 UI를 업데이트해야 하므로 브로드캐스트
		OnCarouselIndexChanged.Broadcast(CarouselIndex);
	}
}

void ACrazyRooGameState::OnRep_CarouselIndex()
{
	// 클라이언트 UI 업데이트를 위해 브로드캐스트
	OnCarouselIndexChanged.Broadcast(CarouselIndex);
}

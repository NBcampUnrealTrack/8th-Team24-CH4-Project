// CrazyRoo 플레이어 스테이트 클래스
#include "Player/CrazyRooPlayerState.h"
#include "CrazyRoo.h"

ACrazyRooPlayerState::ACrazyRooPlayerState()
{
}

void ACrazyRooPlayerState::BeginPlay()
{
	Super::BeginPlay();
	const FString Msg = FString::Printf(TEXT("PlayerState BeginPlay (%s)"),
		*GetName());
	FCrazyRooDebug::Print(this, Msg, FColor::Yellow);
}

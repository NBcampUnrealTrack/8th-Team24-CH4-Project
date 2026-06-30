// CrazyRoo 플레이어 스테이트 클래스
#include "Player/CrazyRooPlayerState.h"
#include "CrazyRoo.h"
#include "Net/UnrealNetwork.h"

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

void ACrazyRooPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACrazyRooPlayerState, PlayerRole);
}

ECrazyRooPlayerRole ACrazyRooPlayerState::GetPlayerRole() const
{
	return PlayerRole;
}

// [서버 전용] 역할 설정. 클라에서 호출되면 무시, 값 같아도 무시.
void ACrazyRooPlayerState::SetPlayerRole(ECrazyRooPlayerRole NewRole)
{
	if (!HasAuthority() || PlayerRole == NewRole)
	{
		return;
	}

	PlayerRole = NewRole;

	// 서버(데디) 측 로그 — 서버 로그창에서 확인
	const FString Msg = FString::Printf(TEXT("[SERVER] SetPlayerRole: %s -> %s"),
		*GetName(), CrazyRooRoleToString(NewRole));
	FCrazyRooDebug::Print(this, Msg, FColor::Yellow);
}

// 복제된 값이 클라에 도착하면 자동 호출 — 이게 호출 되야 "복제 성공"
void ACrazyRooPlayerState::OnRep_PlayerRole()
{
	const FString Msg = FString::Printf(TEXT("[CLIENT] OnRep_PlayerRole = %s"),
	CrazyRooRoleToString(PlayerRole));
	FCrazyRooDebug::Print(this, Msg, FColor::Yellow);
}

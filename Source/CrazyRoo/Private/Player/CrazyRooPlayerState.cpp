// CrazyRoo 플레이어 스테이트 클래스
#include "Player/CrazyRooPlayerState.h"
#include "CrazyRoo.h"
#include "Types/CrazyRooTypes.h"
#include "Net/UnrealNetwork.h"

ACrazyRooPlayerState::ACrazyRooPlayerState()
{
	PlayerRole = ECrazyRooPlayerRole::None;
}

void ACrazyRooPlayerState::BeginPlay()
{
	Super::BeginPlay();
	const FString Msg = FString::Printf(TEXT("PlayerState BeginPlay (%s)"),
		*GetName());
	FCrazyRooDebug::Print(this, Msg, FColor::Yellow);
}

// 복제할 변수를 등록
void ACrazyRooPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACrazyRooPlayerState, PlayerRole);
}

ECrazyRooPlayerRole ACrazyRooPlayerState::GetPlayerRole() const
{
	return PlayerRole;
}

// [Server] 서버의 GameMode가 역할을 최종 승인했을 때 값을 덮어씌우는 함수
void ACrazyRooPlayerState::SetPlayerRole(ECrazyRooPlayerRole NewRole)
{
	// 서버가 아니거나, 바꾸려는 값이 똑같으면 무시
	if (!HasAuthority() || PlayerRole == NewRole)
	{
		return;
	}

	PlayerRole = NewRole; // 데이터 변경

	// 서버(데디) 측 로그 — 서버 로그창에서 확인
	const FString Msg = FString::Printf(TEXT("[SERVER] 플레이어-%d 역할 -> %s"),
		JoinOrder, CrazyRooRoleToString(NewRole));
	FCrazyRooDebug::Print(this, Msg, FColor::Yellow);
}

int32 ACrazyRooPlayerState::GetJoinOrder() const
{
	return JoinOrder;
}

void ACrazyRooPlayerState::SetJoinOrder(int32 InJoinOrder)
{
	if (!HasAuthority())
	{
		return;
	}
	JoinOrder = InJoinOrder;
}

// [Client] 서버에서 역할 값이 바뀌어 내 컴퓨터로 전송되었을 때 자동 호출되는 함수
void ACrazyRooPlayerState::OnRep_PlayerRole()
{
	// 내 화면(클라이언트)에도 역할이 동기화되었음을 확인하는 로그
	const FString Msg = FString::Printf(TEXT("[CLIENT] 플레이어-%d 역할 = %s"),
		JoinOrder, CrazyRooRoleToString(PlayerRole));
	FCrazyRooDebug::Print(this, Msg, FColor::Yellow);

	// (OnRoleChanged.Broadcast 등을 이용해 UI를 업데이트 할 수 있는 위치입니다)
}

// CrazyRoo 게임 인스턴스 클래스
#include "System/CrazyRooGameInstance.h"
#include "CrazyRoo.h"

UCrazyRooGameInstance::UCrazyRooGameInstance()
{
}

void UCrazyRooGameInstance::Init()
{
	Super::Init();
	// GameInstance 는 프로세스마다 1개. World 가 아직 없을 수 있어 Print 대신 UE_LOG 직접 사용.
	UE_LOG(LogCrazyRoo, Warning, TEXT("GameInstance Init (프로세스마다 1개)"));
}

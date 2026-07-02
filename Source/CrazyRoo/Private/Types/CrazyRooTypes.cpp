// CrazyRoo - System Framework & Multiplayer
// CrazyRooTypes.h 에 선언된 헬퍼 함수 구현부.

#include "Types/CrazyRooTypes.h"

// 비대칭 조작 역할 Enum을 문자열 변환
const TCHAR* CrazyRooRoleToString(ECrazyRooPlayerRole R)
{
	switch (R)
	{
	case ECrazyRooPlayerRole::P_Left:    return TEXT("P_Left");
	case ECrazyRooPlayerRole::P_Right:   return TEXT("P_Right");
	case ECrazyRooPlayerRole::P_Shooter: return TEXT("P_Shooter");
	case ECrazyRooPlayerRole::P_Acrobat: return TEXT("P_Acrobat");
	default:                              return TEXT("None");
	}
}

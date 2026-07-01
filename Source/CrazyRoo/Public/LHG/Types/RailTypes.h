// RailTypes.h

#pragma once

#include "CoreMinimal.h"
#include "RailTypes.generated.h"

// 4명의 플레이어 역할을 구분하기 위한 enum
UENUM(BlueprintType)
enum class ERailPlayerRole : uint8
{
	LeftControl,
	RightControl,
	JumpSlideControl,
	AimShootControl
};

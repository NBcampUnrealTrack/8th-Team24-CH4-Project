// RailInputConfig.h

#pragma once

#include"CoreMinimal.h"
#include"Engine/DataAsset.h"
#include"RailInputConfig.generated.h"

class UInputMappingContext;
class UInputAction;

/**
 * 입력 관련 에셋을 한 곳에서 관리하는 Data Asset 클래스
 * PlayerController가 Input Action을 하나씩 들고 있지 않고,
 * 이 Config 하나만 참조하도록 만들기 위한 클래스
 */
UCLASS(BlueprintType)
class CRAZYROO_API URailInputConfig :public UDataAsset
{
	GENERATED_BODY()

public:
	// 이 플레이어에게 적용할 Enhanced Input Mapping Context
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Input")
	UInputMappingContext* InputMappingContext;

	// 왼쪽 레일 변경 / 좌회전 입력
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Input|Action")
	UInputAction* IA_Left;

	// 오른쪽 레일 변경 / 우회전 입력
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Input|Action")
	UInputAction* IA_Right;

	// 점프 입력
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Input|Action")
	UInputAction* IA_Jump;

	// 슬라이딩 입력
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Input|Action")
	UInputAction* IA_Slide;

	// 사격 입력
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Input|Action")
	UInputAction* IA_Fire;

	// Q 키: 카메라 좌측 90도 회전
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Action")
	UInputAction* IA_CameraLeft;

	// E 키: 카메라 우측 90도 회전
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Action")
	UInputAction* IA_CameraRight;

	// [한길] 0703 테스트용 앞 뒤 움직임.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	UInputAction* IA_Move;
};

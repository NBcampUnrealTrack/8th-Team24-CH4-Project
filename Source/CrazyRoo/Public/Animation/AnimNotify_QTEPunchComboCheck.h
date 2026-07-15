#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_QTEPunchComboCheck.generated.h"

UCLASS()
class CRAZYROO_API UAnimNotify_QTEPunchComboCheck : public UAnimNotify
{
	GENERATED_BODY()

public:
	// 펀치 Section 끝에서 호출되어 다음 예약 입력이 있으면 다음 펀치로 이어갑니다.
	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference
	) override;
};

#include "Animation/AnimNotify_QTEPunchComboCheck.h"

#include "Components/SkeletalMeshComponent.h"
#include "Player/SharedRailCharacter.h"

void UAnimNotify_QTEPunchComboCheck::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference
)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	// QTE 펀치 몽타주는 공유 캐릭터 Mesh에서 재생되므로 Owner를 캐릭터로 변환해 콤보를 이어갑니다.
	if (ASharedRailCharacter* SharedCharacter = Cast<ASharedRailCharacter>(MeshComp->GetOwner()))
	{
		SharedCharacter->HandleQTEPunchComboCheck();
	}
}

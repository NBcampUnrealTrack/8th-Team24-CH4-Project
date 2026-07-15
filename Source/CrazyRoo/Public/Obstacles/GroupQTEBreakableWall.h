// GroupQTEBreakableWall.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Types/CrazyRooTypes.h"
#include "GroupQTEBreakableWall.generated.h"

class ASharedCharacterManager;
class ASharedRailCharacter;
class AFlyingEnemy;
class AFlyingEnemySpawner;
class UBoxComponent;
class UCameraComponent;
class UCrazyRooQTEWidget;
class UMaterialInstanceDynamic;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

USTRUCT(BlueprintType)
struct FQTERolePressCount
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="QTE")
	ECrazyRooPlayerRole Role = ECrazyRooPlayerRole::None;

	UPROPERTY(BlueprintReadOnly, Category="QTE")
	int32 PressCount = 0;
};

UENUM(BlueprintType)
enum class EGroupQTEState : uint8
{
	Idle UMETA(DisplayName="Idle"),
	Preparing UMETA(DisplayName="Preparing"),
	Active UMETA(DisplayName="Active"),
	Success UMETA(DisplayName="Success"),
	Failed UMETA(DisplayName="Failed")
};

UCLASS(Blueprintable)
class CRAZYROO_API AGroupQTEBreakableWall : public AActor
{
	GENERATED_BODY()

public:
	AGroupQTEBreakableWall();

	void HandleQTEPress(ECrazyRooPlayerRole RequestRole);

	UFUNCTION(BlueprintPure, Category="QTE")
	bool IsQTEActive() const { return bQTEActive; }

	UFUNCTION(BlueprintPure, Category="QTE")
	bool IsQTEPreparing() const { return QTEState == EGroupQTEState::Preparing; }

	UFUNCTION(BlueprintPure, Category="QTE")
	EGroupQTEState GetQTEState() const { return QTEState; }

	UFUNCTION(BlueprintPure, Category="QTE")
	bool IsWallBroken() const { return bWallBroken; }

	UFUNCTION(BlueprintPure, Category="QTE")
	int32 GetCurrentTotalPressCount() const { return CurrentTotalPressCount; }

	UFUNCTION(BlueprintPure, Category="QTE")
	int32 GetRequiredTotalPressCount() const { return RequiredTotalPressCount; }

	UFUNCTION(BlueprintPure, Category="QTE")
	float GetQTEProgressRatio() const;

	UFUNCTION(BlueprintPure, Category="QTE")
	int32 GetCurrentProgressStage() const { return CurrentProgressStage; }

	UFUNCTION(BlueprintPure, Category="QTE")
	float GetQTERemainingTime() const;

	UFUNCTION(BlueprintPure, Category="QTE")
	FText GetQTERemainingTimeText() const;

	UFUNCTION(BlueprintPure, Category="QTE")
	const TArray<FQTERolePressCount>& GetRolePressCounts() const { return RolePressCounts; }

	UFUNCTION(BlueprintPure, Category="QTE")
	int32 GetPressCountForRole(ECrazyRooPlayerRole PlayerRole) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> WallMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UBoxComponent> TriggerBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCameraComponent> QTECamera;

	// 수동으로 설정하는 팀 전체 입력 목표 수입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category="QTE", meta=(ClampMin="1"))
	int32 RequiredTotalPressCount = 30;

	// 입력을 받기 전 카메라/UI를 보여주는 준비 시간입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="QTE", meta=(ClampMin="0.0", Units="s"))
	float PreparationDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="QTE", meta=(ClampMin="0.1", Units="s"))
	float TimeLimit = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="QTE")
	float FailureDamage = 9999.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="QTE")
	bool bDestroyOnBreak = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="QTE|UI")
	TSubclassOf<UCrazyRooQTEWidget> QTEWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="QTE|Camera", meta=(ClampMin="0.0", Units="s"))
	float CameraBlendTime = 0.5f;

	// 벽 균열/파괴 연출을 몇 단계로 나눌지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="QTE|Feedback", meta=(ClampMin="1"))
	int32 ProgressStageCount = 4;

	// 단계별로 교체할 벽 메시입니다. 1단계부터 순서대로 넣으면 됩니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="QTE|Feedback")
	TArray<TObjectPtr<UStaticMesh>> ProgressStageMeshes;

	// 벽 머티리얼에 전달할 진행도 파라미터 이름입니다. 비워두면 사용하지 않습니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="QTE|Feedback")
	FName WallDamageMaterialParameterName = TEXT("QTEProgress");

	// QTE 입력 중 캐릭터 애니메이션이 너무 자주 끊기지 않도록 재생 간격을 제한합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="QTE|Feedback", meta=(ClampMin="0.0", Units="s"))
	float QTEPunchAnimationCooldown = 0.15f;

	UPROPERTY(ReplicatedUsing=OnRep_QTEActive, BlueprintReadOnly, Category="QTE")
	bool bQTEActive = false;

	UPROPERTY(ReplicatedUsing=OnRep_QTEState, BlueprintReadOnly, Category="QTE")
	EGroupQTEState QTEState = EGroupQTEState::Idle;

	UPROPERTY(ReplicatedUsing=OnRep_WallBroken, BlueprintReadOnly, Category="QTE")
	bool bWallBroken = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="QTE")
	int32 CurrentTotalPressCount = 0;

	// 준비 중에는 준비 종료 시간, 입력 중에는 제한 시간 종료 시간을 뜻합니다.
	UPROPERTY(Replicated, BlueprintReadOnly, Category="QTE")
	float QTEEndServerTimeSeconds = 0.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="QTE")
	TArray<FQTERolePressCount> RolePressCounts;

	UPROPERTY(ReplicatedUsing=OnRep_CurrentProgressStage, BlueprintReadOnly, Category="QTE|Feedback")
	int32 CurrentProgressStage = 0;

	UPROPERTY()
	TObjectPtr<UStaticMesh> OriginalWallStaticMesh;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicWallMaterials;

	UPROPERTY()
	TArray<TWeakObjectPtr<AFlyingEnemySpawner>> PausedEnemySpawners;

	UPROPERTY()
	TObjectPtr<UCrazyRooQTEWidget> ActiveQTEWidget;

	float LastQTEPunchAnimationTime = -999.0f;

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void HandleTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	void StartQTE();
	void BeginActiveQTE();
	void CompleteQTE();
	void FailQTE();
	void BreakWall();
	void ApplyWallBrokenState();
	void PauseEnemySpawningForQTE();
	void ClearActiveFlyingEnemiesForQTE();
	void ResumeEnemySpawningAfterQTE();
	void ResumeRunnerMovementAfterQTE(ASharedCharacterManager* Manager);

	UFUNCTION()
	void OnRep_QTEActive();

	UFUNCTION()
	void OnRep_QTEState();

	UFUNCTION()
	void OnRep_CurrentProgressStage();

	UFUNCTION()
	void OnRep_WallBroken();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ShowQTEWidget();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_HideQTEWidget(bool bRestoreCamera);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayQTEPressFeedback(ECrazyRooPlayerRole PressRole, int32 NewTotalPressCount, int32 NewProgressStage);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_QTEProgressStageChanged(int32 NewProgressStage);

	UFUNCTION(BlueprintImplementableEvent, Category="QTE|Feedback")
	void BP_OnQTEStateChanged(EGroupQTEState NewState);

	UFUNCTION(BlueprintImplementableEvent, Category="QTE|Feedback")
	void BP_OnQTEPressFeedback(ECrazyRooPlayerRole PressRole, int32 NewTotalPressCount, int32 NewProgressStage);

	UFUNCTION(BlueprintImplementableEvent, Category="QTE|Feedback")
	void BP_OnQTEProgressStageChanged(int32 NewProgressStage);

	void ShowQTEWidget();
	void HideQTEWidget();
	void SwitchToQTECamera();
	void RestoreGameplayCamera();
	void SetQTEState(EGroupQTEState NewState);
	void UpdateProgressStage();
	void TryPlayQTEPunchAnimation();
	void InitializeWallVisuals();
	void ApplyWallProgressStageVisuals();
	void InitializeRolePressCounts();
	void AddRolePressCount(ECrazyRooPlayerRole PlayerRole);
	FString GetRolePressCountsLog() const;
	void LogQTEProgress(const TCHAR* EventName, ECrazyRooPlayerRole RequestRole = ECrazyRooPlayerRole::None) const;

	ASharedCharacterManager* GetSharedCharacterManager() const;
	ASharedRailCharacter* GetSharedCharacter() const;

private:
	FTimerHandle QTETimerHandle;
	FTimerHandle QTEPreparationTimerHandle;
};

// GroupQTEBreakableWall.cpp

#include "Obstacles/GroupQTEBreakableWall.h"

#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Enemies/FlyingEnemy.h"
#include "Enemies/FlyingEnemySpawner.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Manager/SharedCharacterManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "Player/SharedRailCharacter.h"
#include "System/CrazyRooGameMode.h"
#include "TimerManager.h"
#include "UI/CrazyRooQTEWidget.h"

AGroupQTEBreakableWall::AGroupQTEBreakableWall()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	WallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WallMesh"));
	WallMesh->SetupAttachment(SceneRoot);
	WallMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WallMesh->SetCollisionObjectType(ECC_WorldStatic);
	WallMesh->SetCollisionResponseToAllChannels(ECR_Block);

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(SceneRoot);
	TriggerBox->SetBoxExtent(FVector(250.0f, 550.0f, 250.0f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->SetGenerateOverlapEvents(true);

	QTECamera = CreateDefaultSubobject<UCameraComponent>(TEXT("QTECamera"));
	QTECamera->SetupAttachment(SceneRoot);
	QTECamera->SetRelativeLocation(FVector(1170.0f, -360.0f, 100.0f));
	QTECamera->SetRelativeRotation(FRotator(13.0f, -198.0f, 13.0f));
	QTECamera->bConstrainAspectRatio = false;
}

void AGroupQTEBreakableWall::BeginPlay()
{
	Super::BeginPlay();

	if (TriggerBox)
	{
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AGroupQTEBreakableWall::HandleTriggerBeginOverlap);
	}

	InitializeWallVisuals();
	InitializeRolePressCounts();
}

void AGroupQTEBreakableWall::HandleTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	if (!HasAuthority() || bWallBroken || QTEState != EGroupQTEState::Idle)
	{
		return;
	}

	if (Cast<ASharedRailCharacter>(OtherActor))
	{
		StartQTE();
	}
}

void AGroupQTEBreakableWall::StartQTE()
{
	if (!HasAuthority() || bWallBroken || QTEState != EGroupQTEState::Idle)
	{
		return;
	}

	ApplyRequiredPressCountByPlayerCount();
	SetQTEState(EGroupQTEState::Preparing);
	CurrentTotalPressCount = 0;
	CurrentProgressStage = 0;
	LastQTEPunchAnimationTime = -999.0f;
	ApplyWallProgressStageVisuals();
	QTEEndServerTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() + PreparationDuration : PreparationDuration;
	InitializeRolePressCounts();
	PauseEnemySpawningForQTE();
	ClearActiveFlyingEnemiesForQTE();

	if (ASharedRailCharacter* SharedCharacter = GetSharedCharacter())
	{
		SharedCharacter->StopMovementForQTE();
		SharedCharacter->MoveToCenterLane();
	}

	if (ASharedCharacterManager* Manager = GetSharedCharacterManager())
	{
		Manager->StopMove();
		Manager->SetRunnerInputLocked(true);
		Manager->RegisterActiveQTEWall(this);
	}

	ForceNetUpdate();
	Multicast_ShowQTEWidget();

	if (PreparationDuration <= 0.0f)
	{
		BeginActiveQTE();
	}
	else
	{
		GetWorldTimerManager().SetTimer(
			QTEPreparationTimerHandle,
			this,
			&AGroupQTEBreakableWall::BeginActiveQTE,
			PreparationDuration,
			false
		);
	}

	UE_LOG(LogTemp, Warning, TEXT("[QTE] Preparing: Required=%d Preparation=%.1f TimeLimit=%.1f"),
		RequiredTotalPressCount,
		PreparationDuration,
		TimeLimit);
	LogQTEProgress(TEXT("Preparing"));
}

void AGroupQTEBreakableWall::BeginActiveQTE()
{
	if (!HasAuthority() || bWallBroken || QTEState != EGroupQTEState::Preparing)
	{
		return;
	}

	SetQTEState(EGroupQTEState::Active);
	QTEEndServerTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() + TimeLimit : TimeLimit;
	ForceNetUpdate();

	GetWorldTimerManager().SetTimer(
		QTETimerHandle,
		this,
		&AGroupQTEBreakableWall::FailQTE,
		TimeLimit,
		false
	);

	UE_LOG(LogTemp, Warning, TEXT("[QTE] Active: Required=%d TimeLimit=%.1f"), RequiredTotalPressCount, TimeLimit);
	LogQTEProgress(TEXT("Active"));
}

void AGroupQTEBreakableWall::HandleQTEPress(ECrazyRooPlayerRole RequestRole)
{
	if (!HasAuthority() || QTEState != EGroupQTEState::Active || bWallBroken)
	{
		return;
	}

	if (RequestRole == ECrazyRooPlayerRole::None)
	{
		return;
	}

	CurrentTotalPressCount++;
	AddRolePressCount(RequestRole);
	TryPlayQTEPunchAnimation();
	UpdateProgressStage();
	Multicast_PlayQTEPressFeedback(RequestRole, CurrentTotalPressCount, CurrentProgressStage);

	LogQTEProgress(TEXT("Press"), RequestRole);

	if (CurrentTotalPressCount >= RequiredTotalPressCount)
	{
		CompleteQTE();
	}
}

void AGroupQTEBreakableWall::CompleteQTE()
{
	if (!HasAuthority() || QTEState != EGroupQTEState::Active)
	{
		return;
	}

	SetQTEState(EGroupQTEState::Success);
	CurrentProgressStage = ProgressStageCount;
	ApplyWallProgressStageVisuals();
	Multicast_QTEProgressStageChanged(CurrentProgressStage);
	QTEEndServerTimeSeconds = 0.0f;
	GetWorldTimerManager().ClearTimer(QTEPreparationTimerHandle);
	GetWorldTimerManager().ClearTimer(QTETimerHandle);
	ForceNetUpdate();
	Multicast_HideQTEWidget(true);

	if (ASharedRailCharacter* SharedCharacter = GetSharedCharacter())
	{
		SharedCharacter->Multicast_StopQTEPunchMontage();
	}

	if (ASharedCharacterManager* Manager = GetSharedCharacterManager())
	{
		Manager->UnregisterActiveQTEWall(this);
		Manager->SetRunnerInputLocked(false);
		ResumeRunnerMovementAfterQTE(Manager);
	}

	ResumeEnemySpawningAfterQTE();

	LogQTEProgress(TEXT("Complete"));
	BreakWall();
}

void AGroupQTEBreakableWall::FailQTE()
{
	if (!HasAuthority() || QTEState != EGroupQTEState::Active)
	{
		return;
	}

	SetQTEState(EGroupQTEState::Failed);
	QTEEndServerTimeSeconds = 0.0f;
	GetWorldTimerManager().ClearTimer(QTEPreparationTimerHandle);
	GetWorldTimerManager().ClearTimer(QTETimerHandle);
	ForceNetUpdate();
	Multicast_HideQTEWidget(false);

	if (ASharedRailCharacter* SharedCharacter = GetSharedCharacter())
	{
		SharedCharacter->Multicast_StopQTEPunchMontage();
	}

	if (ASharedCharacterManager* Manager = GetSharedCharacterManager())
	{
		Manager->UnregisterActiveQTEWall(this);
	}

	LogQTEProgress(TEXT("Failed"));

	if (ASharedRailCharacter* SharedCharacter = GetSharedCharacter())
	{
		SharedCharacter->ApplyPlayerDamage(FailureDamage, this, true);
	}
}

void AGroupQTEBreakableWall::BreakWall()
{
	if (!HasAuthority() || bWallBroken)
	{
		return;
	}

	bWallBroken = true;
	Multicast_HideQTEWidget(true);
	ApplyWallBrokenState();

	if (bDestroyOnBreak)
	{
		Destroy();
	}
}

void AGroupQTEBreakableWall::ApplyWallBrokenState()
{
	if (!bWallBroken)
	{
		return;
	}

	// Chaos GeometryCollection은 BP에서 자식 컴포넌트로 붙여 쓰므로 액터 전체를 숨기거나 충돌을 끄지 않습니다.
	// 통과 판정은 기존 WallMesh/TriggerBox만 비활성화하고, 파괴 파편 연출은 BP 컴포넌트가 계속 보여주게 둡니다.
	if (WallMesh)
	{
		WallMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WallMesh->SetHiddenInGame(true);
	}

	if (TriggerBox)
	{
		TriggerBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AGroupQTEBreakableWall::OnRep_QTEActive()
{
	if (bQTEActive)
	{
		SwitchToQTECamera();
		ShowQTEWidget();
	}
	else
	{
		HideQTEWidget();
	}
}

void AGroupQTEBreakableWall::OnRep_QTEState()
{
	BP_OnQTEStateChanged(QTEState);

	switch (QTEState)
	{
	case EGroupQTEState::Preparing:
	case EGroupQTEState::Active:
		SwitchToQTECamera();
		ShowQTEWidget();
		break;

	case EGroupQTEState::Success:
	case EGroupQTEState::Failed:
	case EGroupQTEState::Idle:
	default:
		HideQTEWidget();
		break;
	}
}

void AGroupQTEBreakableWall::OnRep_CurrentProgressStage()
{
	ApplyWallProgressStageVisuals();
	BP_OnQTEProgressStageChanged(CurrentProgressStage);
}

void AGroupQTEBreakableWall::OnRep_WallBroken()
{
	HideQTEWidget();
	RestoreGameplayCamera();
	ApplyWallBrokenState();
}

void AGroupQTEBreakableWall::Multicast_ShowQTEWidget_Implementation()
{
	SwitchToQTECamera();
	ShowQTEWidget();
	BP_OnQTEStateChanged(QTEState);
}

void AGroupQTEBreakableWall::Multicast_HideQTEWidget_Implementation(bool bRestoreCamera)
{
	HideQTEWidget();
	if (bRestoreCamera)
	{
		RestoreGameplayCamera();
	}
	BP_OnQTEStateChanged(QTEState);
}

void AGroupQTEBreakableWall::Multicast_PlayQTEPressFeedback_Implementation(
	ECrazyRooPlayerRole PressRole,
	int32 NewTotalPressCount,
	int32 NewProgressStage
)
{
	BP_OnQTEPressFeedback(PressRole, NewTotalPressCount, NewProgressStage);
}

void AGroupQTEBreakableWall::Multicast_QTEProgressStageChanged_Implementation(int32 NewProgressStage)
{
	ApplyWallProgressStageVisuals();
	BP_OnQTEProgressStageChanged(NewProgressStage);
}

void AGroupQTEBreakableWall::ShowQTEWidget()
{
	const UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_DedicatedServer || !QTEWidgetClass)
	{
		return;
	}

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	if (!ActiveQTEWidget)
	{
		ActiveQTEWidget = CreateWidget<UCrazyRooQTEWidget>(PlayerController, QTEWidgetClass);
		if (ActiveQTEWidget)
		{
			ActiveQTEWidget->AddToViewport();
		}
	}

	if (ActiveQTEWidget)
	{
		ActiveQTEWidget->SetQTEWall(this);
	}
}

void AGroupQTEBreakableWall::HideQTEWidget()
{
	if (!ActiveQTEWidget)
	{
		return;
	}

	ActiveQTEWidget->RemoveFromParent();
	ActiveQTEWidget = nullptr;
}

void AGroupQTEBreakableWall::SwitchToQTECamera()
{
	const UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_DedicatedServer || !QTECamera)
	{
		return;
	}

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	QTECamera->SetActive(true);
	PlayerController->SetViewTargetWithBlend(this, CameraBlendTime);
}

void AGroupQTEBreakableWall::RestoreGameplayCamera()
{
	const UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	if (ASharedRailCharacter* SharedCharacter = GetSharedCharacter())
	{
		if (SharedCharacter->IsPlayerDead())
		{
			return;
		}

		PlayerController->SetViewTargetWithBlend(SharedCharacter, CameraBlendTime);
	}
}

void AGroupQTEBreakableWall::PauseEnemySpawningForQTE()
{
	if (!HasAuthority())
	{
		return;
	}

	PausedEnemySpawners.Reset();

	TArray<AActor*> FoundSpawners;
	UGameplayStatics::GetAllActorsOfClass(this, AFlyingEnemySpawner::StaticClass(), FoundSpawners);
	for (AActor* Actor : FoundSpawners)
	{
		AFlyingEnemySpawner* Spawner = Cast<AFlyingEnemySpawner>(Actor);
		if (!IsValid(Spawner) || !Spawner->IsSpawning())
		{
			continue;
		}

		PausedEnemySpawners.Add(Spawner);
		Spawner->StopSpawning();
	}
}

void AGroupQTEBreakableWall::ClearActiveFlyingEnemiesForQTE()
{
	if (!HasAuthority())
	{
		return;
	}

	TArray<AActor*> FoundEnemies;
	UGameplayStatics::GetAllActorsOfClass(this, AFlyingEnemy::StaticClass(), FoundEnemies);
	for (AActor* Actor : FoundEnemies)
	{
		AFlyingEnemy* Enemy = Cast<AFlyingEnemy>(Actor);
		if (!IsValid(Enemy))
		{
			continue;
		}

		Enemy->DisableEnemyInteraction();
		Enemy->Destroy();
	}
}

void AGroupQTEBreakableWall::ResumeEnemySpawningAfterQTE()
{
	if (!HasAuthority())
	{
		return;
	}

	for (const TWeakObjectPtr<AFlyingEnemySpawner>& SpawnerPtr : PausedEnemySpawners)
	{
		if (SpawnerPtr.IsValid())
		{
			SpawnerPtr->StartSpawning();
		}
	}

	PausedEnemySpawners.Reset();
}

void AGroupQTEBreakableWall::ResumeRunnerMovementAfterQTE(ASharedCharacterManager* Manager)
{
	if (!HasAuthority() || !Manager)
	{
		return;
	}

	const ACrazyRooGameMode* CrazyRooGameMode = GetWorld()
		? GetWorld()->GetAuthGameMode<ACrazyRooGameMode>()
		: nullptr;

	const ECrazyRooGameStartMode StartMode = CrazyRooGameMode
		? CrazyRooGameMode->GetGameStartMode()
		: ECrazyRooGameStartMode::Normal;

	switch (StartMode)
	{
	case ECrazyRooGameStartMode::ManualTest:
		Manager->StartManualMove();
		break;

	case ECrazyRooGameStartMode::Normal:
	case ECrazyRooGameStartMode::AutoMoveTest:
	default:
		Manager->StartAutoMove();
		break;
	}
}

void AGroupQTEBreakableWall::SetQTEState(EGroupQTEState NewState)
{
	if (QTEState == NewState)
	{
		return;
	}

	QTEState = NewState;
	bQTEActive = QTEState == EGroupQTEState::Active;
	BP_OnQTEStateChanged(QTEState);
}

void AGroupQTEBreakableWall::ApplyRequiredPressCountByPlayerCount()
{
	if (!HasAuthority() || RequiredPressCountByPlayerCount.IsEmpty())
	{
		return;
	}

	// 배열 규칙: 1명은 Index 0, 2명은 Index 1, 3명은 Index 2를 사용합니다.
	// 배열보다 플레이어 수가 많으면 마지막 값을 사용해서 설정 누락으로 인한 실패를 막습니다.
	const int32 PlayerCount = GetQTEPlayerCount();
	const int32 TargetIndex = FMath::Clamp(PlayerCount - 1, 0, RequiredPressCountByPlayerCount.Num() - 1);
	const int32 NewRequiredPressCount = RequiredPressCountByPlayerCount[TargetIndex];

	if (NewRequiredPressCount <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[QTE] PlayerCount target ignored: PlayerCount=%d Index=%d Value=%d"),
			PlayerCount,
			TargetIndex,
			NewRequiredPressCount);
		return;
	}

	RequiredTotalPressCount = NewRequiredPressCount;
	UE_LOG(LogTemp, Warning, TEXT("[QTE] Required count set by player count: PlayerCount=%d Required=%d"),
		PlayerCount,
		RequiredTotalPressCount);
}

int32 AGroupQTEBreakableWall::GetQTEPlayerCount() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 1;
	}

	int32 PlayerCount = 0;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (It->Get())
		{
			PlayerCount++;
		}
	}

	// 테스트 중 PlayerController를 아직 못 찾는 경우에도 최소 1명 기준으로 계산합니다.
	return FMath::Max(1, PlayerCount);
}

void AGroupQTEBreakableWall::UpdateProgressStage()
{
	const int32 PreviousStage = CurrentProgressStage;
	const float ProgressRatio = GetQTEProgressRatio();
	CurrentProgressStage = FMath::Clamp(
		FMath::FloorToInt(ProgressRatio * ProgressStageCount),
		0,
		ProgressStageCount
	);

	if (CurrentProgressStage != PreviousStage)
	{
		ApplyWallProgressStageVisuals();
		Multicast_QTEProgressStageChanged(CurrentProgressStage);
	}
}

void AGroupQTEBreakableWall::TryPlayQTEPunchAnimation()
{
	if (!HasAuthority())
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float CurrentTime = World->GetTimeSeconds();
	// 콤보 입력은 G키 입력 횟수만큼 예약되어야 하므로, 쿨타임을 0보다 크게 설정한 경우에만 입력 예약을 제한합니다.
	if (QTEPunchAnimationCooldown > 0.0f && CurrentTime - LastQTEPunchAnimationTime < QTEPunchAnimationCooldown)
	{
		return;
	}

	if (ASharedRailCharacter* SharedCharacter = GetSharedCharacter())
	{
		SharedCharacter->Multicast_PlayQTEPunchMontage();
		LastQTEPunchAnimationTime = CurrentTime;
	}
}

void AGroupQTEBreakableWall::InitializeWallVisuals()
{
	if (!WallMesh)
	{
		return;
	}

	OriginalWallStaticMesh = WallMesh->GetStaticMesh();
	DynamicWallMaterials.Reset();

	const int32 MaterialCount = WallMesh->GetNumMaterials();
	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		UMaterialInstanceDynamic* DynamicMaterial = WallMesh->CreateAndSetMaterialInstanceDynamic(MaterialIndex);
		if (DynamicMaterial)
		{
			DynamicWallMaterials.Add(DynamicMaterial);
		}
	}

	ApplyWallProgressStageVisuals();
}

void AGroupQTEBreakableWall::ApplyWallProgressStageVisuals()
{
	if (!WallMesh)
	{
		return;
	}

	if (CurrentProgressStage <= 0)
	{
		if (OriginalWallStaticMesh)
		{
			WallMesh->SetStaticMesh(OriginalWallStaticMesh);
		}
	}
	else
	{
		const int32 StageMeshIndex = CurrentProgressStage - 1;
		if (ProgressStageMeshes.IsValidIndex(StageMeshIndex) && ProgressStageMeshes[StageMeshIndex])
		{
			WallMesh->SetStaticMesh(ProgressStageMeshes[StageMeshIndex]);
		}
	}

	if (!WallDamageMaterialParameterName.IsNone())
	{
		const float ProgressRatio = GetQTEProgressRatio();
		for (UMaterialInstanceDynamic* DynamicMaterial : DynamicWallMaterials)
		{
			if (DynamicMaterial)
			{
				DynamicMaterial->SetScalarParameterValue(WallDamageMaterialParameterName, ProgressRatio);
			}
		}
	}
}

void AGroupQTEBreakableWall::InitializeRolePressCounts()
{
	RolePressCounts.Reset();

	constexpr ECrazyRooPlayerRole Roles[] =
	{
		ECrazyRooPlayerRole::P_Left,
		ECrazyRooPlayerRole::P_Right,
		ECrazyRooPlayerRole::P_Shooter,
		ECrazyRooPlayerRole::P_Acrobat
	};

	for (const ECrazyRooPlayerRole PlayerRole : Roles)
	{
		FQTERolePressCount RolePressCount;
		RolePressCount.Role = PlayerRole;
		RolePressCount.PressCount = 0;
		RolePressCounts.Add(RolePressCount);
	}
}

void AGroupQTEBreakableWall::AddRolePressCount(ECrazyRooPlayerRole PlayerRole)
{
	for (FQTERolePressCount& RolePressCount : RolePressCounts)
	{
		if (RolePressCount.Role == PlayerRole)
		{
			RolePressCount.PressCount++;
			return;
		}
	}

	FQTERolePressCount RolePressCount;
	RolePressCount.Role = PlayerRole;
	RolePressCount.PressCount = 1;
	RolePressCounts.Add(RolePressCount);
}

float AGroupQTEBreakableWall::GetQTEProgressRatio() const
{
	if (RequiredTotalPressCount <= 0)
	{
		return 0.0f;
	}

	return FMath::Clamp(static_cast<float>(CurrentTotalPressCount) / static_cast<float>(RequiredTotalPressCount), 0.0f, 1.0f);
}

float AGroupQTEBreakableWall::GetQTERemainingTime() const
{
	if ((QTEState != EGroupQTEState::Preparing && QTEState != EGroupQTEState::Active) || QTEEndServerTimeSeconds <= 0.0f)
	{
		return 0.0f;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0f;
	}

	float CurrentServerTime = World->GetTimeSeconds();
	if (const AGameStateBase* GameState = World->GetGameState())
	{
		CurrentServerTime = GameState->GetServerWorldTimeSeconds();
	}

	return FMath::Max(0.0f, QTEEndServerTimeSeconds - CurrentServerTime);
}

FText AGroupQTEBreakableWall::GetQTERemainingTimeText() const
{
	const int32 TotalSeconds = FMath::Max(0, FMath::CeilToInt(GetQTERemainingTime()));
	const int32 Minutes = TotalSeconds / 60;
	const int32 Seconds = TotalSeconds % 60;

	return FText::FromString(FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds));
}

int32 AGroupQTEBreakableWall::GetPressCountForRole(ECrazyRooPlayerRole PlayerRole) const
{
	for (const FQTERolePressCount& RolePressCount : RolePressCounts)
	{
		if (RolePressCount.Role == PlayerRole)
		{
			return RolePressCount.PressCount;
		}
	}

	return 0;
}

FString AGroupQTEBreakableWall::GetRolePressCountsLog() const
{
	TArray<FString> Parts;
	for (const FQTERolePressCount& RolePressCount : RolePressCounts)
	{
		Parts.Add(FString::Printf(
			TEXT("%s=%d"),
			CrazyRooRoleToString(RolePressCount.Role),
			RolePressCount.PressCount
		));
	}

	return FString::Join(Parts, TEXT(", "));
}

void AGroupQTEBreakableWall::LogQTEProgress(const TCHAR* EventName, ECrazyRooPlayerRole RequestRole) const
{
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[QTE][%s] State=%d InputRole=%s Total=%d/%d Progress=%.2f Stage=%d/%d Remaining=%.2fs Roles={%s}"),
		EventName,
		static_cast<int32>(QTEState),
		CrazyRooRoleToString(RequestRole),
		CurrentTotalPressCount,
		RequiredTotalPressCount,
		GetQTEProgressRatio(),
		CurrentProgressStage,
		ProgressStageCount,
		GetQTERemainingTime(),
		*GetRolePressCountsLog()
	);
}

ASharedCharacterManager* AGroupQTEBreakableWall::GetSharedCharacterManager() const
{
	return Cast<ASharedCharacterManager>(
		UGameplayStatics::GetActorOfClass(this, ASharedCharacterManager::StaticClass())
	);
}

ASharedRailCharacter* AGroupQTEBreakableWall::GetSharedCharacter() const
{
	return Cast<ASharedRailCharacter>(
		UGameplayStatics::GetActorOfClass(this, ASharedRailCharacter::StaticClass())
	);
}

void AGroupQTEBreakableWall::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGroupQTEBreakableWall, bQTEActive);
	DOREPLIFETIME(AGroupQTEBreakableWall, QTEState);
	DOREPLIFETIME(AGroupQTEBreakableWall, bWallBroken);
	DOREPLIFETIME(AGroupQTEBreakableWall, RequiredTotalPressCount);
	DOREPLIFETIME(AGroupQTEBreakableWall, CurrentTotalPressCount);
	DOREPLIFETIME(AGroupQTEBreakableWall, QTEEndServerTimeSeconds);
	DOREPLIFETIME(AGroupQTEBreakableWall, RolePressCounts);
	DOREPLIFETIME(AGroupQTEBreakableWall, CurrentProgressStage);
}

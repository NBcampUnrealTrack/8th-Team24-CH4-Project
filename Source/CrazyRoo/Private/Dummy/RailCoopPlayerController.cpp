// // RailCoopPlayerController.cpp
//
// #include "Dummy/RailCoopPlayerController.h"
// #include "Dummy/RailCoopGameMode.h"
// #include "Manager/SharedCharacterManager.h"
// #include "EnhancedInputComponent.h"
// #include "EnhancedInputSubsystems.h"
// #include "Net/UnrealNetwork.h"
// #include "Input/RailInputConfig.h"
// #include "Dummy/RailTypes.h"
// #include "Player/SharedRailCharacter.h"
// #include "Kismet/GameplayStatics.h"
// #include "Engine/World.h"
// #include "Engine/LocalPlayer.h"
//
// ARailCoopPlayerController::ARailCoopPlayerController()
// {
//     bReplicates =true;
//
//     // 기본 역할
//     PlayerRole = ERailPlayerRole::LeftControl;
// }
//
// void ARailCoopPlayerController::BeginPlay()
// {
//     Super::BeginPlay();
//
//     UE_LOG(LogTemp, Warning, TEXT("RailCoopPlayerController BeginPlay"));
//
//     // 입력 등록은 로컬 플레이어에서만 실행한다
//     // 서버나 다른 클라이언트의 PlayerController에서는 입력을 받을 필요가 없음
//     if (!IsLocalController())
//     {
//         UE_LOG(LogTemp, Error, TEXT("Not Local Controller"));
//         return;
//     }
//
//     // 마우스 커서 보이게 하기.
//     FInputModeGameAndUI InputMode;
//     InputMode.SetHideCursorDuringCapture(false);
//     InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
//
//     SetInputMode(InputMode);
//     bShowMouseCursor = true;
//
//     // InputConfig가 비어 있으면 입력 세팅 불가
//     if (!InputConfig)
//     {
//         UE_LOG(LogTemp,Warning,TEXT("InputConfig is missing."));
//         return;
//     }
//
//     // InputConfig 안에 Mapping Context가 비어 있으면 등록 불가
//     if (!InputConfig->InputMappingContext)
//     {
//         UE_LOG(LogTemp,Warning,TEXT("InputMappingContext is missing in InputConfig."));
//         return;
//     }
//
//     ULocalPlayer* LocalPlayer = GetLocalPlayer();
//
//     if (!LocalPlayer)
//     {
//         UE_LOG(LogTemp, Error, TEXT("LocalPlayer is NULL"));
//         return;
//     }
//
//     UEnhancedInputLocalPlayerSubsystem* Subsystem =
//     LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
//
//     if (!Subsystem)
//     {
//         UE_LOG(LogTemp, Error, TEXT("EnhancedInput Subsystem is NULL"));
//         return;
//     }
//
//     // 기존 Mapping Context 정리
//     // 테스트 단계에서는 입력 중복을 막기 위해 한번 비워주는 것이 편함
//     Subsystem->ClearAllMappings();
//
//     // InputConfig에 들어 있는 Mapping Context 등록
//     Subsystem->AddMappingContext(InputConfig->InputMappingContext,0);
//
//     UE_LOG(LogTemp, Warning, TEXT("Input Mapping Context Added"));
//
//     // 맵에 배치된 공유 캐릭터를 찾는다
//     ASharedRailCharacter* SharedCharacter = Cast<ASharedRailCharacter>(
//         UGameplayStatics::GetActorOfClass(
//             GetWorld(),
//             ASharedRailCharacter::StaticClass()
//         )
//     );
//
//     if (SharedCharacter)
//     {
//         // 이 PlayerController의 화면을 공유 캐릭터 카메라로 전환
//         SetViewTargetWithBlend(SharedCharacter, 0.0f);
//
//         UE_LOG(LogTemp, Warning, TEXT("ViewTarget set to SharedCharacter"));
//     }
//     else
//     {
//         UE_LOG(LogTemp, Error, TEXT("SharedCharacter not found for ViewTarget"));
//     }
// }
//
// void ARailCoopPlayerController::SetupInputComponent()
// {
//     Super::SetupInputComponent();
//
//     UE_LOG(LogTemp, Warning, TEXT("SetupInputComponent Called"));
//
//     UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
//
//     if (!EnhancedInput)
//     {
//         UE_LOG(LogTemp, Warning, TEXT("EnhancedInputComponent is missing."));
//         return;
//     }
//
//     if (!InputConfig)
//     {
//         UE_LOG(LogTemp, Warning, TEXT("InputConfig is missing in SetupInputComponent."));
//         return;
//     }
//
//     UE_LOG(LogTemp, Warning, TEXT("InputConfig Found in SetupInputComponent"));
//
//     // InputConfig 안에 있는 Input Action들을 바인딩한다
//     // 이제 PlayerController는 개별 IA 변수를 직접 들고 있을 필요가 없다
//
//     if (InputConfig->IA_Left)
//     {
//         EnhancedInput->BindAction(
//             InputConfig->IA_Left,
//             ETriggerEvent::Started,
//             this,
//             &ARailCoopPlayerController::InputLeft
//             );
//         UE_LOG(LogTemp, Warning, TEXT("IA_Left Bound"));
//     }
//     else
//     {
//         UE_LOG(LogTemp, Error, TEXT("IA_Left is NULL"));
//     }
//
//     if (InputConfig->IA_Right)
//     {
//         EnhancedInput->BindAction(
//             InputConfig->IA_Right,
//             ETriggerEvent::Started,
//             this,
//             &ARailCoopPlayerController::InputRight
//             );
//     }
//
//     if (InputConfig->IA_Jump)
//     {
//         EnhancedInput->BindAction(
//             InputConfig->IA_Jump,
//             ETriggerEvent::Started,
//             this,
//             &ARailCoopPlayerController::InputJump
//             );
//     }
//
//     if (InputConfig->IA_Slide)
//     {
//         EnhancedInput->BindAction(
//             InputConfig->IA_Slide,
//             ETriggerEvent::Started,
//             this,
//             &ARailCoopPlayerController::InputSlide
//             );
//     }
//
//     if (InputConfig->IA_Fire)
//     {
//         EnhancedInput->BindAction(
//             InputConfig->IA_Fire,
//             ETriggerEvent::Started,
//             this,
//             &ARailCoopPlayerController::InputFire
//             );
//     }
//
//     if (InputConfig->IA_CameraLeft)
//     {
//         EnhancedInput->BindAction(
//             InputConfig->IA_CameraLeft,
//             ETriggerEvent::Started,
//             this,
//             &ARailCoopPlayerController::InputCameraLeft
//         );
//
//         UE_LOG(LogTemp, Warning, TEXT("IA_CameraLeft Bound"));
//     }
//
//     if (InputConfig->IA_CameraRight)
//     {
//         EnhancedInput->BindAction(
//             InputConfig->IA_CameraRight,
//             ETriggerEvent::Started,
//             this,
//             &ARailCoopPlayerController::InputCameraRight
//         );
//
//         UE_LOG(LogTemp, Warning, TEXT("IA_CameraRight Bound"));
//     }
// }
//
// void ARailCoopPlayerController::SetPlayerRole(ERailPlayerRole NewRole)
// {
//     // 역할 배정은 서버에서만 실행한다
//     if (!HasAuthority())
//     {
//         return;
//     }
//
//     PlayerRole =NewRole;
// }
//
// void ARailCoopPlayerController::InputLeft()
// {
//     UE_LOG(LogTemp, Warning, TEXT("InputLeft Pressed"));
//
//     // 클라이언트 입력을 서버로 전달
//     Server_RequestMoveLeft();
// }
//
// void ARailCoopPlayerController::InputRight()
// {
//     UE_LOG(LogTemp, Warning, TEXT("InputRight Pressed"));
//
//     Server_RequestMoveRight();
// }
//
// void ARailCoopPlayerController::InputJump()
// {
//     UE_LOG(LogTemp, Warning, TEXT("InputJump Pressed"));
//
//     Server_RequestJump();
// }
//
// void ARailCoopPlayerController::InputSlide()
// {
//     UE_LOG(LogTemp, Warning, TEXT("InputSlide Pressed"));
//
//     Server_RequestSlide();
// }
//
// void ARailCoopPlayerController::InputFire()
// {
//     UE_LOG(LogTemp, Warning, TEXT("InputFire Pressed"));
//
//     FVector AimOrigin;
//     FVector AimDirection;
//
//     if (!GetMouseAimRay(AimOrigin, AimDirection))
//     {
//         UE_LOG(LogTemp, Error, TEXT("Failed to get mouse aim ray"));
//         return;
//     }
//
//     Server_RequestFireAimRay(AimOrigin, AimDirection);
// }
//
// void ARailCoopPlayerController::InputCameraLeft()
// {
//     UE_LOG(LogTemp, Warning, TEXT("InputCameraLeft Pressed"));
//
//     Server_RequestRotateCameraLeft();
// }
//
// void ARailCoopPlayerController::InputCameraRight()
// {
//     UE_LOG(LogTemp, Warning, TEXT("InputCameraRight Pressed"));
//
//     Server_RequestRotateCameraRight();
// }
//
// bool ARailCoopPlayerController::GetMouseWorldTarget(FVector& OutTargetLocation) const
// {
//     FVector WorldOrigin;
//     FVector WorldDirection;
//
//     // 마우스 화면 위치를 월드 위치 + 방향으로 변환
//     bool bSuccess = DeprojectMousePositionToWorld(
//         WorldOrigin,
//         WorldDirection
//     );
//
//     if (!bSuccess)
//     {
//         return false;
//     }
//
//     // 캐릭터가 있는 높이 기준으로 바닥 Plane 생성
//     float TargetZ = 0.f;
//
//     if (APawn* MyPawn = GetPawn())
//     {
//         TargetZ = MyPawn->GetActorLocation().Z;
//     }
//
//     // 공유 캐릭터를 직접 찾는 방식
//     ASharedRailCharacter* SharedCharacter =
//         Cast<ASharedRailCharacter>(
//             UGameplayStatics::GetActorOfClass(
//                 GetWorld(),
//                 ASharedRailCharacter::StaticClass()
//             )
//         );
//
//     if (SharedCharacter)
//     {
//         TargetZ = SharedCharacter->GetActorLocation().Z;
//     }
//
//     // 마우스 Ray가 TargetZ 평면과 만나는 지점 계산
//     float T = (TargetZ - WorldOrigin.Z) / WorldDirection.Z;
//
//     if (T < 0.f)
//     {
//         return false;
//     }
//
//     OutTargetLocation = WorldOrigin + WorldDirection * T;
//
//     return true;
// }
//
// bool ARailCoopPlayerController::GetMouseAimRay(
//     FVector& OutOrigin,
//     FVector& OutDirection
// ) const
// {
//     // 화면 위 마우스 위치를 월드 위치와 방향으로 변환
//     bool bSuccess = DeprojectMousePositionToWorld(
//         OutOrigin,
//         OutDirection
//     );
//
//     if (!bSuccess)
//     {
//         return false;
//     }
//
//     OutDirection.Normalize();
//     return true;
// }
//
// ASharedCharacterManager* ARailCoopPlayerController::GetManagerOnServer() const
// {
//     // GameMode는 서버에만 존재한다
//     ARailCoopGameMode* GM = GetWorld()->GetAuthGameMode<ARailCoopGameMode>();
//
//     if (!GM)
//     {
//         return nullptr;
//     }
//
//     return GM->GetSharedCharacterManager();
// }
//
// void ARailCoopPlayerController::Server_RequestMoveLeft_Implementation()
// {
//     UE_LOG(LogTemp, Warning, TEXT("Server_RequestMoveLeft Called"));
//
//     // 서버에서도 역할을 다시 검사한다
//     // 클라이언트 조작이나 버그로 잘못된 요청이 와도 막을 수 있음
//     /*
//     if (PlayerRole!= ERailPlayerRole::LeftControl)
//     {
//         return;
//     }
//     */
//
//
//     if (ASharedCharacterManager* Manager = GetManagerOnServer())
//     {
//         Manager->RequestMoveLeft(PlayerRole);
//     }
//     else
//     {
//         UE_LOG(LogTemp, Error, TEXT("Manager is NULL"));
//     }
// }
//
// void ARailCoopPlayerController::Server_RequestMoveRight_Implementation()
// {
//     if (ASharedCharacterManager* Manager = GetManagerOnServer())
//     {
//         Manager->RequestMoveRight(PlayerRole);
//     }
// }
//
// void ARailCoopPlayerController::Server_RequestJump_Implementation()
// {
//     if (ASharedCharacterManager* Manager = GetManagerOnServer())
//     {
//         Manager->RequestJump(PlayerRole);
//     }
// }
//
// void ARailCoopPlayerController::Server_RequestSlide_Implementation()
// {
//     if (ASharedCharacterManager* Manager = GetManagerOnServer())
//     {
//         Manager->RequestSlide(PlayerRole);
//     }
// }
//
// void ARailCoopPlayerController::Server_RequestFireToLocation_Implementation(FVector TargetLocation)
// {
//     if (ASharedCharacterManager* Manager = GetManagerOnServer())
//     {
//         Manager->RequestFireToLocation(PlayerRole, TargetLocation);
//     }
//     else
//     {
//         UE_LOG(LogTemp, Error, TEXT("Manager is NULL"));
//     }
// }
//
// void ARailCoopPlayerController::Server_RequestRotateCameraLeft_Implementation()
// {
//     UE_LOG(LogTemp, Warning, TEXT("Server_RequestRotateCameraLeft"));
//
//     if (ASharedCharacterManager* Manager = GetManagerOnServer())
//     {
//         Manager->RequestRotateCameraLeft(PlayerRole);
//     }
//     else
//     {
//         UE_LOG(LogTemp, Error, TEXT("Manager is NULL"));
//     }
// }
//
// void ARailCoopPlayerController::Server_RequestRotateCameraRight_Implementation()
// {
//     UE_LOG(LogTemp, Warning, TEXT("Server_RequestRotateCameraRight"));
//
//     if (ASharedCharacterManager* Manager = GetManagerOnServer())
//     {
//         Manager->RequestRotateCameraRight(PlayerRole);
//     }
//     else
//     {
//         UE_LOG(LogTemp, Error, TEXT("Manager is NULL"));
//     }
// }
//
// void ARailCoopPlayerController::Server_RequestFireAimRay_Implementation(
//     FVector_NetQuantize AimOrigin,
//     FVector_NetQuantizeNormal AimDirection
// )
// {
//     if (ASharedCharacterManager* Manager = GetManagerOnServer())
//     {
//         Manager->RequestFireAimRay(
//             PlayerRole,
//             AimOrigin,
//             AimDirection
//         );
//     }
// }
//
// void ARailCoopPlayerController::GetLifetimeReplicatedProps(
// TArray<FLifetimeProperty>&OutLifetimeProps
// )const
// {
//     Super::GetLifetimeReplicatedProps(OutLifetimeProps);
//
//     DOREPLIFETIME(ARailCoopPlayerController,PlayerRole);
// }

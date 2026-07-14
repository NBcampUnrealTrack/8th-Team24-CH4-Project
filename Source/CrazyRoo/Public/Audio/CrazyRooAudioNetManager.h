// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Audio/CrazyRooAudioTypes.h"
#include "GameFramework/Actor.h"
#include "CrazyRooAudioNetManager.generated.h"

class UDataTable;

UCLASS(Blueprintable)
class CRAZYROO_API ACrazyRooAudioNetManager : public AActor
{
	GENERATED_BODY()

public:
	ACrazyRooAudioNetManager();

	UFUNCTION(BlueprintCallable, Category = "CrazyRoo|Audio")
	void RequestPlayBGM(ECrazyRooBGMType BGMType);

	UFUNCTION(BlueprintCallable, Category = "CrazyRoo|Audio")
	void RequestPlaySFXAtLocation(ECrazyRooSFXType SFXType, FVector Location);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayBGM(ECrazyRooBGMType BGMType);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlaySFXAtLocation(ECrazyRooSFXType SFXType, FVector Location);

	UFUNCTION(BlueprintCallable, Category = "CrazyRoo|Audio", meta = (WorldContext = "World"))
	static ACrazyRooAudioNetManager* FindAudioNetManager(UWorld* World);

protected:
	virtual void BeginPlay() override;

	// BGM usage:
	// MainMenu UI Construct or MainMenu BeginPlay -> AudioSubsystem.PlayBGM(MainMenu)
	// MapSelect UI Construct -> AudioSubsystem.PlayBGM(SelectMenu)
	// RoleSelect UI Construct -> AudioSubsystem.PlayBGM(SelectMenu)
	// Gameplay start -> AudioSubsystem.PlayBGM(InGame)
	// Stage clear sequence -> AudioNetManager.RequestPlayBGM(StageClear)
	// Death sequence -> AudioNetManager.RequestPlayBGM(Death)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CrazyRoo|Audio")
	TObjectPtr<UDataTable> BGMDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CrazyRoo|Audio")
	TObjectPtr<UDataTable> SFXDataTable;
};

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Game/DS_MatchGameMode.h"
#include "CombatGameMode.generated.h"

/**
 *  Simple GameMode for a third person combat game
 */
UCLASS(abstract)
class ACombatGameMode : public ADS_MatchGameMode
{
	GENERATED_BODY()
	
public:

	ACombatGameMode();
	virtual void Tick(float DeltaTime) override;
	virtual void StartPlayerElimination(float ElimTime, ACharacter* ElimmedCharacter, class APlayerController* VictimController, APlayerController* AttackerController);

	UPROPERTY()
	TMap<APlayerController*, FTimerHandle> Timers;

	virtual void PlayerEliminated(ACharacter* ElimmedCharacter, class APlayerController* VictimController, APlayerController* AttackerController);
	virtual void RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController);


	UPROPERTY(EditDefaultsOnly, Category = "Respawning")
	float RespawnTime;
	
protected:
	
	virtual void OnMatchEnded() override;
};

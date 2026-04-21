// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Combat/CombatGameMode.h"

#include "Character/MatchPlayerState.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Player/DSPlayerController.h"

ACombatGameMode::ACombatGameMode()
{
	RespawnTime = 15.f;
	PlayerStateClass = AMatchPlayerState::StaticClass();
}

void ACombatGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ACombatGameMode::StartPlayerElimination(float ElimTime, ACharacter* ElimmedCharacter,
	APlayerController* VictimController, APlayerController* AttackerController)
{
	FTimerDelegate ElimTimerDelegate;
	FTimerHandle TimerHandle;
	Timers.Add(VictimController, TimerHandle);
	ElimTimerDelegate.BindLambda([this, ElimmedCharacter, VictimController, AttackerController]()
		{
			PlayerEliminated(ElimmedCharacter, VictimController, AttackerController);
			GetWorldTimerManager().ClearTimer(Timers[VictimController]);
			Timers.Remove(VictimController);

		});
	GetWorldTimerManager().SetTimer(TimerHandle, ElimTimerDelegate, ElimTime + RespawnTime, false);
}

void ACombatGameMode::PlayerEliminated(ACharacter* ElimmedCharacter, APlayerController* VictimController,
	APlayerController* AttackerController)
{
	RequestRespawn(ElimmedCharacter, VictimController);
}

void ACombatGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Reset();
		ElimmedCharacter->Destroy();
	}
	if (ElimmedController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
	}
}

void ACombatGameMode::OnMatchEnded()
{
	Super::OnMatchEnded();
	
	TArray<FString> Usernames;
	if (UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (ADSPlayerController* PC = Cast<ADSPlayerController>(It->Get()); IsValid(PC))
			{
				Usernames.Add(PC->Username);
			}
		}
	}
	
	UpdateLeaderboard(Usernames);
}

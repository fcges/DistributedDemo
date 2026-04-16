// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DS_GameModeBase.h"
#if WITH_GAMELIFT
#include "GameLiftServerSDK.h"
#endif
#include "DS_LobbyGameMode.generated.h"

class UDS_GameInstanceSubsystem;
#if WITH_GAMELIFT
struct FProcessParameters;
#endif
/**
 * 
 */
UCLASS()
class DEDICATEDSERVERS_API ADS_LobbyGameMode : public ADS_GameModeBase
{
	GENERATED_BODY()
	
public:
	
	ADS_LobbyGameMode();
	void CheckAndStartLobbyCountdown();
	virtual void PostLogin(APlayerController* NewPlayer) override;
	
protected:
	
	virtual void BeginPlay() override;
	virtual void OnCountdownTimerFinished(ECountdownTimerType Type) override;
	virtual void InitSeamlessTravelPlayer(AController* NewController) override;
	void CheckAndStopLobbyCountdown();
	virtual void Logout(AController* Exiting) override;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal = L"") override;
	
	UPROPERTY()
	ELobbyStatus LobbyStatus;
	
	UPROPERTY(EditDefaultsOnly)
	int32 MinPlayers;
	
	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UWorld> DestinationMap;
	
private:
	
	UPROPERTY()
	TObjectPtr<UDS_GameInstanceSubsystem> DSGameInstanceSubsystem;
	
	UPROPERTY(EditDefaultsOnly)
	FCountdownTimerHandle LobbyCountdownTimer;
	
#if WITH_GAMELIFT
	void InitGameLift();
	void SetServerParameters(FServerParameters& ServerParametersForAnywhere);
	void TryAcceptPlayerSession(const FString& PlayerSessionId, const FString& Username, FString& OutErrorMessage);
#endif
	
	void AddPlayerInfoToLobbyState(AController* Player) const;
	void RemovePlayerInfoFromLobbyState(AController* Player) const;
};

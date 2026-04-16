// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#if WITH_GAMELIFT
#include "GameLiftServerSDK.h"
#include "GameLiftServerSDKModels.h"
#endif
#include "Subsystems/GameInstanceSubsystem.h"
#include "DS_GameInstanceSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class DEDICATEDSERVERS_API UDS_GameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	
	UDS_GameInstanceSubsystem();
	
	UPROPERTY(BlueprintReadOnly)
	bool bGameLiftInitialized;
	
#if WITH_GAMELIFT
public:
	
	void InitGameLift(const FServerParameters& ServerParams);
	
private:
	TSharedPtr<FProcessParameters> ProcessParameters;
#endif
};

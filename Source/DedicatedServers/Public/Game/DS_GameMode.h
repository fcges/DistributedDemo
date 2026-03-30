// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#if WITH_GAMELIFT
#include "GameLiftServerSDKModels.h"
#endif
#include "DS_GameMode.generated.h"

#if WITH_GAMELIFT
struct FProcessParameters;
#endif

DECLARE_LOG_CATEGORY_EXTERN(LogDS_GameMode, Log, All);

/**
 * 
 */
UCLASS()
class DEDICATEDSERVERS_API ADS_GameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	
protected:
	virtual void BeginPlay() override;
	
private:
#if WITH_GAMELIFT
	void InitGameLift();
	TSharedPtr<FProcessParameters> ProcessParameters;
	void SetServerParameters(FServerParameters& ServerParametersForAnywhere);
#endif
};

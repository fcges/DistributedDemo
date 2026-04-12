// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "PlayerHUD.generated.h"

class UStatusOverlay;
/**
 * 
 */
UCLASS()
class DISTRIBUTEDDEMO_API APlayerHUD : public AHUD
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UStatusOverlay> StatusOverlayClass;
	
protected:
	
	virtual void BeginPlay();
	
private:
	
	UPROPERTY()
	TObjectPtr<UStatusOverlay> StatusOverlay;
};

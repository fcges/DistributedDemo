// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MatchStatsWidget.generated.h"

class UTextBlock;
class AMatchPlayerState;
/**
 * 
 */
UCLASS()
class DISTRIBUTEDDEMO_API UMatchStatsWidget : public UUserWidget
{
	GENERATED_BODY()
	
private:
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextBlock_Stats;

	TWeakObjectPtr<AMatchPlayerState> BoundPlayerState;

	FTimerHandle BindRetryTimerHandle;

	void TryBindToPlayerState();
	void BindToPlayerState(AMatchPlayerState* InPlayerState);
	void UnbindFromPlayerState();

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
	
public:
	UFUNCTION()
	void UpdateStats(int32 Kill, int32 Death, int32 Assist);
};

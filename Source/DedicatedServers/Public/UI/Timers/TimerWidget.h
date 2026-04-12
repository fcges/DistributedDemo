// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/DSTypes.h"
#include "TimerWidget.generated.h"

class UTextBlock;
class ADSPlayerController;

/**
 * 
 */
UCLASS()
class DEDICATEDSERVERS_API UTimerWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	
protected:
	
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	ECountdownTimerType TimerType = ECountdownTimerType::None;
	
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<ADSPlayerController> OwningPlayerController;
	
	UPROPERTY(BlueprintReadOnly)
	bool bActive = false;
	
	UPROPERTY(EditDefaultsOnly)
	bool bCanBeNegative = false;
	
	UPROPERTY(EditDefaultsOnly)
	bool bShowCentiSeconds = false;
	
	UPROPERTY(EditDefaultsOnly)
	bool bHiddenWhenInactive = true;
	
	UFUNCTION()
	virtual void OnTimeUpdated(float CountdownTimeLeft, ECountdownTimerType Type);
	
	UFUNCTION()
	virtual void OnTimeStopped(float CountdownTimeLeft, ECountdownTimerType Type);
	
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnTimerUpdated"))
	void K2_OnTimerUpdated(float Time, ECountdownTimerType Type);
	
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnTimerStopped"))
	void K2_OnTimerStopped(float Time, ECountdownTimerType Type);
	
	FString FormatTimeAsString(float TimeSeconds) const;
	
private:
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextBlock_Time;
	
	float InternalCountdown;
	
	void TimerStarted(float InitialTime);
	void TimerStopped();
	void UpdateCountdown(float TimeSeconds);
};

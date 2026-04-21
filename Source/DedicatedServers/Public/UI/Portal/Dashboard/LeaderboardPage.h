// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LeaderboardPage.generated.h"

struct FDSLeaderboardItem;
class UTextBlock;
class ULeaderboardCard;
class UScrollBox;
/**
 * 
 */
UCLASS()
class DEDICATEDSERVERS_API ULeaderboardPage : public UUserWidget
{
	GENERATED_BODY()
	
public:
	
	UFUNCTION()
	void PopulateLeaderboard(TArray<FDSLeaderboardItem>& Leaderboard);
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> ScrollBox_Leaderboard;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextBlock_StatusMessage;
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ULeaderboardCard> LeaderboardCardClass;
	
	UFUNCTION()
	void SetStatusMessage(const FString& StatusMessage, bool bShouldResetWidgets);
	
private:
	
	void CalculatedLeaderboardPlaces(TArray<FDSLeaderboardItem>& OutLeaderboard);
};

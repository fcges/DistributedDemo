// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Portal/Dashboard/LeaderboardPage.h"

#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "UI/HTTP/HTTPRequestTypes.h"
#include "UI/Portal/Dashboard/LeaderboardCard.h"

void ULeaderboardPage::PopulateLeaderboard(TArray<FDSLeaderboardItem>& Leaderboard)
{
	ScrollBox_Leaderboard->ClearChildren();
	CalculatedLeaderboardPlaces(Leaderboard);
	
	for (const FDSLeaderboardItem& Item : Leaderboard)
	{
		ULeaderboardCard* LeaderboardCard = CreateWidget<ULeaderboardCard>(this, LeaderboardCardClass);
		if (IsValid(LeaderboardCard))
		{
			LeaderboardCard->SetPlayerInfo(Item.username, Item.place, Item.score);
			ScrollBox_Leaderboard->AddChild(LeaderboardCard);
		}
	}
}

void ULeaderboardPage::CalculatedLeaderboardPlaces(TArray<FDSLeaderboardItem>& OutLeaderboard)
{
	OutLeaderboard.Sort([](const FDSLeaderboardItem& A, const FDSLeaderboardItem& B)
	{
		return A.score > B.score;
	});
	
	// Assign places
	int32 CurrentRank = 1;
	for (int32 i = 0; i < OutLeaderboard.Num(); ++i)
	{
		if (i > 0 && OutLeaderboard[i].score == OutLeaderboard[i - 1].score)
		{
			OutLeaderboard[i].place = OutLeaderboard[i - 1].place; // Same place for same score
		}
		else
		{
			OutLeaderboard[i].place = CurrentRank; // Assign current rank as place
		}
		CurrentRank++;
	}
}

void ULeaderboardPage::SetStatusMessage(const FString& StatusMessage, bool bShouldResetWidgets)
{
	TextBlock_StatusMessage->SetText(FText::FromString(StatusMessage));
}

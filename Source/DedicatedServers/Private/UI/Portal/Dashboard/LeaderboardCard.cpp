// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Portal/Dashboard/LeaderboardCard.h"

#include "Components/TextBlock.h"

void ULeaderboardCard::SetPlayerInfo(const FString& Username, int32 Place, int32 Score) const
{
	TextBlock_Username->SetText(FText::FromString(Username));
	TextBlock_Place->SetText(FText::AsNumber(Place));
	TextBlock_Score->SetText(FText::AsNumber(Score));
}

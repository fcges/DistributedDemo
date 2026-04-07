// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/API/GameSessions/JoinGameWidget.h"

#include "Components/TextBlock.h"

void UJoinGameWidget::SetStatusMessage(const FString& Message) const
{
	TextBlock_StatusMessage->SetText(FText::FromString(Message));
}

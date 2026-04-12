// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Portal/Dashboard/GamePage.h"

#include "Components/Button.h"
#include "UI/API/GameSessions/JoinGameWidget.h"
#include "UI/GameSessions/GameSessionManager.h"

void UGamePage::NativeConstruct()
{
	Super::NativeConstruct();
	
	GameSessionManager = NewObject<UGameSessionManager>(this, GameSessionManagerClass);
	GameSessionManager->BroadcastJoinGameSessionMessage.AddDynamic(JoinGameWidget, &UJoinGameWidget::SetStatusMessage);
	
	JoinGameWidget->Button_JoinGame->OnClicked.AddDynamic(this, &UGamePage::JoinGameButtonClicked);
}

void UGamePage::JoinGameButtonClicked()
{
	JoinGameWidget->Button_JoinGame->SetIsEnabled(false);
	GameSessionManager->JoinGameSession();
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Player/PlayerHUD.h"
#include "UI/Player/StatusOverlay.h"
#include "Blueprint/UserWidget.h"

void APlayerHUD::BeginPlay()
{
	Super::BeginPlay();
	
	APlayerController* OwningPlayerController = GetOwningPlayerController();
	StatusOverlay = CreateWidget<UStatusOverlay>(OwningPlayerController, StatusOverlayClass);
	if (IsValid(StatusOverlay))
	{
		StatusOverlay->AddToViewport();
	}
}

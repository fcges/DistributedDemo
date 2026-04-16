// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Player/MatchStatsWidget.h"

#include "Character/MatchPlayerState.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"

void UMatchStatsWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TryBindToPlayerState();

	if (!BoundPlayerState.IsValid())
	{
		GetWorld()->GetTimerManager().SetTimer(BindRetryTimerHandle, this, &UMatchStatsWidget::TryBindToPlayerState, 0.25f, true);
	}
}

void UMatchStatsWidget::NativeDestruct()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(BindRetryTimerHandle);
	}

	UnbindFromPlayerState();
	Super::NativeDestruct();
}

void UMatchStatsWidget::TryBindToPlayerState()
{
	APlayerController* OwningPC = GetOwningPlayer();
	if (!OwningPC)
	{
		return;
	}

	AMatchPlayerState* CurrentPlayerState = Cast<AMatchPlayerState>(OwningPC->PlayerState);
	if (!CurrentPlayerState)
	{
		return;
	}

	BindToPlayerState(CurrentPlayerState);

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(BindRetryTimerHandle);
	}
}

void UMatchStatsWidget::BindToPlayerState(AMatchPlayerState* InPlayerState)
{
	if (!IsValid(InPlayerState) || BoundPlayerState.Get() == InPlayerState)
	{
		return;
	}

	UnbindFromPlayerState();
	BoundPlayerState = InPlayerState;
	InPlayerState->OnMatchStatsChanged().AddUObject(this, &UMatchStatsWidget::UpdateStats);
	UpdateStats(InPlayerState->GetKillCount(), InPlayerState->GetDeathCount(), InPlayerState->GetAssistCount());
}

void UMatchStatsWidget::UnbindFromPlayerState()
{
	if (AMatchPlayerState* CurrentState = BoundPlayerState.Get())
	{
		CurrentState->OnMatchStatsChanged().RemoveAll(this);
	}

	BoundPlayerState.Reset();
}

void UMatchStatsWidget::UpdateStats(int32 Kill, int32 Death, int32 Assist)
{
	if (!TextBlock_Stats)
	{
		return;
	}

	TextBlock_Stats->SetText(FText::FromString(FString::Printf(TEXT("Kill: %d  Death: %d  Assist: %d"), Kill, Death, Assist)));
}

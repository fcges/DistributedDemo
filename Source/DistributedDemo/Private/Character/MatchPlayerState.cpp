// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/MatchPlayerState.h"

#include "Net/UnrealNetwork.h"
#include "UI/HTTP/HTTPRequestTypes.h"

AMatchPlayerState::AMatchPlayerState()
	: KillCount(0)
	, DeathCount(0)
	, AssistCount(0)
{
}

void AMatchPlayerState::OnMatchEnded(const FString& Username)
{
	Super::OnMatchEnded(Username);
	
	FDSRecordMatchStatsInput RecordMatchStatsInput;
	RecordMatchStatsInput.username = Username;
	RecordMatchStatsInput.matchStats.kills = KillCount;
	RecordMatchStatsInput.matchStats.deaths = DeathCount;
	RecordMatchStatsInput.matchStats.assists = AssistCount;
	
	RecordMatchStats(RecordMatchStatsInput);
}

void AMatchPlayerState::AddKill()
{
	++KillCount;
	BroadcastStatsChanged();
}

void AMatchPlayerState::AddDeath()
{
	++DeathCount;
	BroadcastStatsChanged();
}

void AMatchPlayerState::AddAssist()
{
	++AssistCount;
	BroadcastStatsChanged();
}

void AMatchPlayerState::OnRep_KillCount()
{
	BroadcastStatsChanged();
}

void AMatchPlayerState::OnRep_DeathCount()
{
	BroadcastStatsChanged();
}

void AMatchPlayerState::OnRep_AssistCount()
{
	BroadcastStatsChanged();
}

void AMatchPlayerState::BroadcastStatsChanged()
{
	MatchStatsChangedDelegate.Broadcast(KillCount, DeathCount, AssistCount);
}

void AMatchPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMatchPlayerState, KillCount);
	DOREPLIFETIME(AMatchPlayerState, DeathCount);
	DOREPLIFETIME(AMatchPlayerState, AssistCount);
}


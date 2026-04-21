// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/DS_MatchPlayerState.h"
#include "MatchPlayerState.generated.h"

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnMatchStatsChanged, int32, int32, int32);

/**
 * 
 */
UCLASS()
class DISTRIBUTEDDEMO_API AMatchPlayerState : public ADS_MatchPlayerState
{
	GENERATED_BODY()

public:
	AMatchPlayerState();
	virtual void OnMatchEnded(const FString& Username) override;

	void AddKill();
	void AddDeath();
	void AddAssist();

	int32 GetKillCount() const { return KillCount; }
	int32 GetDeathCount() const { return DeathCount; }
	int32 GetAssistCount() const { return AssistCount; }
	FOnMatchStatsChanged& OnMatchStatsChanged() { return MatchStatsChangedDelegate; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_KillCount();

	UFUNCTION()
	void OnRep_DeathCount();

	UFUNCTION()
	void OnRep_AssistCount();

	void BroadcastStatsChanged();

	UPROPERTY(ReplicatedUsing = OnRep_KillCount, BlueprintReadOnly, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	int32 KillCount;

	UPROPERTY(ReplicatedUsing = OnRep_DeathCount, BlueprintReadOnly, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	int32 DeathCount;

	UPROPERTY(ReplicatedUsing = OnRep_AssistCount, BlueprintReadOnly, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	int32 AssistCount;

	FOnMatchStatsChanged MatchStatsChangedDelegate;
};

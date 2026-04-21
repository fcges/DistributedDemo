// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/HTTP/HTTPRequestManager.h"
#include "Interfaces/IHttpRequest.h"
#include "GameStatsManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRetrieveLeaderboard, TArray<FDSLeaderboardItem>&, Leaderboard);

struct FDSRecordMatchStatsInput;

/**
 * 
 */
UCLASS()
class DEDICATEDSERVERS_API UGameStatsManager : public UHTTPRequestManager
{
	GENERATED_BODY()
	
public:
	
	void RecordMatchStats(const FDSRecordMatchStatsInput& RecordMatchStatsInput);
	void UpdateLeaderboard(const TArray<FString>& Usernames);
	void RetrieveLeaderboard();
	
	UPROPERTY(BlueprintAssignable)
	FOnAPIRequestSucceeded OnUpdateLeaderboardSucceeded;
	
	UPROPERTY(BlueprintAssignable)
	FAPIStatusMessage RetrieveLeaderboardStatusMessage;
	
	UPROPERTY(BlueprintAssignable)
	FOnRetrieveLeaderboard OnRetrieveLeaderboard;
	
private:
	
	void RecordMatchStats_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void UpdateLeaderboard_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void RetrieveLeaderboard_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};

// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/DS_LobbyGameMode.h"

#include "Game/DSGameState.h"
#include "Game/DS_GameInstanceSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Lobby/LobbyState.h"
#include "Player/DSPlayerController.h"
#if WITH_GAMELIFT
#include "DedicatedServers/DedicatedServers.h"
#endif

ADS_LobbyGameMode::ADS_LobbyGameMode()
{
    bUseSeamlessTravel = true;
    LobbyStatus = ELobbyStatus::WaitingForPlayers;
    MinPlayers = 1;
    LobbyCountdownTimer.Type = ECountdownTimerType::LobbyCountdown;
}

void ADS_LobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    
    CheckAndStartLobbyCountdown();
}

void ADS_LobbyGameMode::InitSeamlessTravelPlayer(AController* NewController)
{
    Super::InitSeamlessTravelPlayer(NewController);
    
    CheckAndStartLobbyCountdown();
    
    if (LobbyStatus != ELobbyStatus::SeamlessTravelling)
    {
        AddPlayerInfoToLobbyState(NewController);
    }
    
}

void ADS_LobbyGameMode::Logout(AController* Exiting)
{
    Super::Logout(Exiting);
    
    CheckAndStopLobbyCountdown();
    RemovePlayerSession(Exiting);
    
    if (LobbyStatus != ELobbyStatus::SeamlessTravelling)
    {
        RemovePlayerInfoFromLobbyState(Exiting);
    }
    
}

void ADS_LobbyGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId,
    FString& ErrorMessage)
{
    Super::PreLogin(Options, Address, UniqueId, ErrorMessage);


    const FString PlayerSessionId = UGameplayStatics::ParseOption(Options, TEXT("PlayerSessionId"));
    const FString Username = UGameplayStatics::ParseOption(Options, TEXT("Username"));

#if WITH_GAMELIFT
    TryAcceptPlayerSession(PlayerSessionId, Username, ErrorMessage);
#endif
}

FString ADS_LobbyGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId,
    const FString& Options, const FString& Portal)
{
    FString InitializedString = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);
    

    const FString PlayerSessionId = UGameplayStatics::ParseOption(Options, TEXT("PlayerSessionId"));
    const FString Username = UGameplayStatics::ParseOption(Options, TEXT("Username"));
    
#if WITH_GAMELIFT
    if (ADSPlayerController* DSPlayerController = Cast<ADSPlayerController>(NewPlayerController); IsValid(DSPlayerController))
    {
        DSPlayerController->Username = Username;
        DSPlayerController->PlayerSessionId = PlayerSessionId;
    }
#endif
    
    if (LobbyStatus != ELobbyStatus::SeamlessTravelling)
    {
        AddPlayerInfoToLobbyState(NewPlayerController);
    }
    
    return InitializedString;
}

void ADS_LobbyGameMode::AddPlayerInfoToLobbyState(AController* Player) const
{
    ADSPlayerController* DSPlayerController = Cast<ADSPlayerController>(Player);
    ADSGameState* DSGameState = GetGameState<ADSGameState>();
    if (IsValid(DSGameState) && IsValid(DSGameState->LobbyState) && IsValid(DSPlayerController))
    {
        FLobbyPlayerInfo PlayerInfo(DSPlayerController->Username);
        DSGameState->LobbyState->AddPlayerInfo(PlayerInfo);
    }
}

void ADS_LobbyGameMode::RemovePlayerInfoFromLobbyState(AController* Player) const
{
    ADSPlayerController* DSPlayerController = Cast<ADSPlayerController>(Player);
    ADSGameState* DSGameState = GetGameState<ADSGameState>();
    if (IsValid(DSGameState) && IsValid(DSGameState->LobbyState) && IsValid(DSPlayerController))
    {
        DSGameState->LobbyState->RemovePlayerInfo(DSPlayerController->Username);
    }
}

#if WITH_GAMELIFT
void ADS_LobbyGameMode::TryAcceptPlayerSession(const FString& PlayerSessionId, const FString& Username,
    FString& OutErrorMessage)
{
    if (PlayerSessionId.IsEmpty() || Username.IsEmpty())
    {
        OutErrorMessage = TEXT("Missing PlayerSessionId or Username in connection options.");
        return;
    }
    

    Aws::GameLift::Server::Model::DescribePlayerSessionsRequest DescribePlayerSessionsRequest;
    DescribePlayerSessionsRequest.SetPlayerSessionId(TCHAR_TO_ANSI(*PlayerSessionId));

    const auto& DescribePlayerSessionsOutcome = Aws::GameLift::Server::DescribePlayerSessions(DescribePlayerSessionsRequest);
    if (!DescribePlayerSessionsOutcome.IsSuccess())
    {
        OutErrorMessage = TEXT("Failed to describe player sessions.");
        return;
    }

    const auto& DescribePlayerSessionResult = DescribePlayerSessionsOutcome.GetResult();
    int32 Count = 0;
    const Aws::GameLift::Server::Model::PlayerSession* PlayerSessions = DescribePlayerSessionResult.GetPlayerSessions(Count);
    if (PlayerSessions == nullptr || Count == 0)
    {
        OutErrorMessage = TEXT("No player sessions found for the given PlayerSessionId.");
        return;
    }

    for (int32 i = 0; i < Count; i++)
    {
        const Aws::GameLift::Server::Model::PlayerSession& PlayerSession = PlayerSessions[i];
        if (!Username.Equals(PlayerSession.GetPlayerId())) continue;
        if (PlayerSession.GetStatus() != Aws::GameLift::Server::Model::PlayerSessionStatus::RESERVED)
        {
            OutErrorMessage = FString::Printf(TEXT("Session for %s not RESERVED; fail PreLogin."), *Username);
            return;
        }

        const auto& AcceptPlayerSessionOutcome = Aws::GameLift::Server::AcceptPlayerSession(TCHAR_TO_ANSI(*PlayerSessionId));
        OutErrorMessage = AcceptPlayerSessionOutcome.IsSuccess() ? TEXT("") : FString::Printf(TEXT("Failed to accept player session for %s."), *Username);
        return;
    }

}
#endif

void ADS_LobbyGameMode::CheckAndStopLobbyCountdown()
{
    if (GetNumPlayers() - 1 < MinPlayers && LobbyStatus == ELobbyStatus::CountdownToSeamlessTravel)
    {
        LobbyStatus = ELobbyStatus::WaitingForPlayers;
        StopCountdownTimer(LobbyCountdownTimer);
    }
}

void ADS_LobbyGameMode::CheckAndStartLobbyCountdown()
{
    if (GetNumPlayers() >= MinPlayers && LobbyStatus == ELobbyStatus::WaitingForPlayers)
    {
        LobbyStatus = ELobbyStatus::CountdownToSeamlessTravel;
        StartCountdownTimer(LobbyCountdownTimer);
    }
}

void ADS_LobbyGameMode::BeginPlay()
{
	Super::BeginPlay();
	
#if WITH_GAMELIFT
	InitGameLift();
#endif
}

void ADS_LobbyGameMode::OnCountdownTimerFinished(ECountdownTimerType Type)
{
    Super::OnCountdownTimerFinished(Type);
    
    if (Type == ECountdownTimerType::LobbyCountdown)
    {
        StopCountdownTimer(LobbyCountdownTimer);
        LobbyStatus = ELobbyStatus::SeamlessTravelling;
        TrySeamlessTravel(DestinationMap);
    }
}

#if WITH_GAMELIFT
void ADS_LobbyGameMode::InitGameLift()
{
	if (UGameInstance* GameInstance = GetGameInstance(); IsValid(GameInstance))
	{
		if (DSGameInstanceSubsystem = GameInstance->GetSubsystem<UDS_GameInstanceSubsystem>(); IsValid(DSGameInstanceSubsystem))
		{
			FServerParameters ServerParameters;
		    SetServerParameters(ServerParameters);
		    DSGameInstanceSubsystem->InitGameLift(ServerParameters);
		}
	}
}

void ADS_LobbyGameMode::SetServerParameters(FServerParameters& ServerParametersForAnywhere)
{
    bool bIsAnywhereActive = false;
    if (FParse::Param(FCommandLine::Get(), TEXT("glAnywhere")))
    {
        bIsAnywhereActive = true;
    }
    if (bIsAnywhereActive)
    {
        UE_LOG(LogDedicatedServers, Log, TEXT("Configuring server parameters for Anywhere..."));
        // If GameLift Anywhere is enabled, parse command line arguments and pass them in the ServerParameters object.
        FString glAnywhereWebSocketUrl = "";
        if (FParse::Value(FCommandLine::Get(), TEXT("glAnywhereWebSocketUrl="), glAnywhereWebSocketUrl))
        {
            ServerParametersForAnywhere.m_webSocketUrl = TCHAR_TO_UTF8(*glAnywhereWebSocketUrl);
        }

        FString glAnywhereFleetId = "";
        if (FParse::Value(FCommandLine::Get(), TEXT("glAnywhereFleetId="), glAnywhereFleetId))
        {
            ServerParametersForAnywhere.m_fleetId = TCHAR_TO_UTF8(*glAnywhereFleetId);
        }

        FString glAnywhereProcessId = "";
        if (FParse::Value(FCommandLine::Get(), TEXT("glAnywhereProcessId="), glAnywhereProcessId))
        {
            ServerParametersForAnywhere.m_processId = TCHAR_TO_UTF8(*glAnywhereProcessId);
        }
        else
        {
            // If no ProcessId is passed as a command line argument, generate a randomized unique string.
            FString TimeString = FString::FromInt(std::time(nullptr));
            FString ProcessId = "ProcessId_" + TimeString;
            ServerParametersForAnywhere.m_processId = TCHAR_TO_UTF8(*ProcessId);
        }

        FString glAnywhereHostId = "";
        if (FParse::Value(FCommandLine::Get(), TEXT("glAnywhereHostId="), glAnywhereHostId))
        {
            ServerParametersForAnywhere.m_hostId = TCHAR_TO_UTF8(*glAnywhereHostId);
        }

        FString glAnywhereAuthToken = "";
        if (FParse::Value(FCommandLine::Get(), TEXT("glAnywhereAuthToken="), glAnywhereAuthToken))
        {
            ServerParametersForAnywhere.m_authToken = TCHAR_TO_UTF8(*glAnywhereAuthToken);
        }

        FString glAnywhereAwsRegion = "";
        if (FParse::Value(FCommandLine::Get(), TEXT("glAnywhereAwsRegion="), glAnywhereAwsRegion))
        {
            ServerParametersForAnywhere.m_awsRegion = TCHAR_TO_UTF8(*glAnywhereAwsRegion);
        }

        FString glAnywhereAccessKey = "";
        if (FParse::Value(FCommandLine::Get(), TEXT("glAnywhereAccessKey="), glAnywhereAccessKey))
        {
            ServerParametersForAnywhere.m_accessKey = TCHAR_TO_UTF8(*glAnywhereAccessKey);
        }

        FString glAnywhereSecretKey = "";
        if (FParse::Value(FCommandLine::Get(), TEXT("glAnywhereSecretKey="), glAnywhereSecretKey))
        {
            ServerParametersForAnywhere.m_secretKey = TCHAR_TO_UTF8(*glAnywhereSecretKey);
        }

        FString glAnywhereSessionToken = "";
        if (FParse::Value(FCommandLine::Get(), TEXT("glAnywhereSessionToken="), glAnywhereSessionToken))
        {
            ServerParametersForAnywhere.m_sessionToken = TCHAR_TO_UTF8(*glAnywhereSessionToken);
        }
        // End Set Parameters
        
        UE_LOG(LogDedicatedServers, SetColor, TEXT("%s"), COLOR_YELLOW);
        UE_LOG(LogDedicatedServers, Log, TEXT(">>>> WebSocket URL: %s"), *ServerParametersForAnywhere.m_webSocketUrl);
        UE_LOG(LogDedicatedServers, Log, TEXT(">>>> Fleet ID: %s"), *ServerParametersForAnywhere.m_fleetId);
        UE_LOG(LogDedicatedServers, Log, TEXT(">>>> Process ID: %s"), *ServerParametersForAnywhere.m_processId);
        UE_LOG(LogDedicatedServers, Log, TEXT(">>>> Host ID (Compute Name): %s"), *ServerParametersForAnywhere.m_hostId);
        UE_LOG(LogDedicatedServers, Log, TEXT(">>>> Auth Token: %s"), *ServerParametersForAnywhere.m_authToken);
        UE_LOG(LogDedicatedServers, Log, TEXT(">>>> Aws Region: %s"), *ServerParametersForAnywhere.m_awsRegion);
        UE_LOG(LogDedicatedServers, Log, TEXT(">>>> Access Key: %s"), *ServerParametersForAnywhere.m_accessKey);
        UE_LOG(LogDedicatedServers, Log, TEXT(">>>> Secret Key: %s"), *ServerParametersForAnywhere.m_secretKey);
        UE_LOG(LogDedicatedServers, Log, TEXT(">>>> Session Token: %s"), *ServerParametersForAnywhere.m_sessionToken);
        UE_LOG(LogDedicatedServers, SetColor, TEXT("%s"), COLOR_NONE);
    }
}
#endif

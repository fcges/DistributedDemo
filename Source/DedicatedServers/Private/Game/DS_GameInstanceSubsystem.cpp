// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/DS_GameInstanceSubsystem.h"
#if WITH_GAMELIFT
#include "GameLiftServerSDK.h"
#include "GameLiftServerSDKModels.h"
#include "DedicatedServers/DedicatedServers.h"
#endif

UDS_GameInstanceSubsystem::UDS_GameInstanceSubsystem()
{
	bGameLiftInitialized = false;
}

#if WITH_GAMELIFT
void UDS_GameInstanceSubsystem::InitGameLift(const FServerParameters& ServerParams)
{
	if (bGameLiftInitialized) return;
	
	UE_LOG(LogDedicatedServers, Log, TEXT("Calling InitGameLift..."));
	
	FGameLiftServerSDKModule* GameLiftSdkModule = &FModuleManager::LoadModuleChecked<FGameLiftServerSDKModule>(FName("GameLiftServerSDK"));
	
	UE_LOG(LogDedicatedServers, Log, TEXT("Initializing the GameLift Server..."));

	//InitSDK will establish a local connection with GameLift's agent to enable further communication.
	// Use InitSDK() for EC2, InitSDK(ServerParametersForAnywhere) for Anywhere.
	FGameLiftGenericOutcome InitSdkOutcome = GameLiftSdkModule->InitSDK(ServerParams);
	
	if (InitSdkOutcome.IsSuccess())
    {
        UE_LOG(LogDedicatedServers, SetColor, TEXT("%s"), COLOR_GREEN);
        UE_LOG(LogDedicatedServers, Log, TEXT("GameLift InitSDK succeeded!"));
        UE_LOG(LogDedicatedServers, SetColor, TEXT("%s"), COLOR_NONE);
    }
    else
    {
        UE_LOG(LogDedicatedServers, SetColor, TEXT("%s"), COLOR_RED);
        UE_LOG(LogDedicatedServers, Log, TEXT("ERROR: InitSDK failed : ("));
        FGameLiftError GameLiftError = InitSdkOutcome.GetError();
        UE_LOG(LogDedicatedServers, Log, TEXT("ERROR: %s"), *GameLiftError.m_errorMessage);
        UE_LOG(LogDedicatedServers, SetColor, TEXT("%s"), COLOR_NONE);
        return;
    }
    
    ProcessParameters = MakeShared<FProcessParameters>();
    
    //When a game session is created, Amazon GameLift Servers sends an activation request to the game server and passes along the game session object containing game properties and other settings.
    //Here is where a game server should take action based on the game session object.
    //Once the game server is ready to receive incoming player connections, it should invoke GameLiftServerAPI.ActivateGameSession()
    ProcessParameters->OnStartGameSession.BindLambda([=](Aws::GameLift::Server::Model::GameSession InGameSession)
        {
            FString GameSessionId = FString(InGameSession.GetGameSessionId());
            UE_LOG(LogDedicatedServers, Log, TEXT("GameSession Initializing: %s"), *GameSessionId);
            GameLiftSdkModule->ActivateGameSession();
        });
    
    //OnProcessTerminate callback. Amazon GameLift Servers will invoke this callback before shutting down an instance hosting this game server.
    //It gives this game server a chance to save its state, communicate with services, etc., before being shut down.
    //In this case, we simply tell Amazon GameLift Servers we are indeed going to shutdown.
    ProcessParameters->OnTerminate.BindLambda([=]()
        {
            UE_LOG(LogDedicatedServers, Log, TEXT("Game Server Process is terminating"));
            // First call ProcessEnding()
            FGameLiftGenericOutcome processEndingOutcome = GameLiftSdkModule->ProcessEnding();
            // Then call Destroy() to free the SDK from memory
            FGameLiftGenericOutcome destroyOutcome = GameLiftSdkModule->Destroy();
            // Exit the process with success or failure
            if (processEndingOutcome.IsSuccess() && destroyOutcome.IsSuccess()) {
                UE_LOG(LogDedicatedServers, Log, TEXT("Server process ending successfully"));
            }
            else {
                if (!processEndingOutcome.IsSuccess()) {
                    const FGameLiftError& error = processEndingOutcome.GetError();
                    UE_LOG(LogDedicatedServers, Error, TEXT("ProcessEnding() failed. Error: %s"),
                    error.m_errorMessage.IsEmpty() ? TEXT("Unknown error") : *error.m_errorMessage);
                }
                if (!destroyOutcome.IsSuccess()) {
                    const FGameLiftError& error = destroyOutcome.GetError();
                    UE_LOG(LogDedicatedServers, Error, TEXT("Destroy() failed. Error: %s"),
                    error.m_errorMessage.IsEmpty() ? TEXT("Unknown error") : *error.m_errorMessage);
                }
            }
        });
    
    //This is the HealthCheck callback.
    //Amazon GameLift Servers will invoke this callback every 60 seconds or so.
    //Here, a game server might want to check the health of dependencies and such.
    //Simply return true if healthy, false otherwise.
    //The game server has 60 seconds to respond with its health status. Amazon GameLift Servers will default to 'false' if the game server doesn't respond in time.
    //In this case, we're always healthy!
    ProcessParameters->OnHealthCheck.BindLambda([]()
        {
            UE_LOG(LogDedicatedServers, Log, TEXT("Performing Health Check"));
            return true;
        });
    
    //GameServer.exe -port=7777 LOG=server.mylog
    ProcessParameters->port = FURL::UrlConfig.DefaultPort;
    TArray<FString> CommandLineTokens;
    TArray<FString> CommandLineSwitches;

    FCommandLine::Parse(FCommandLine::Get(), CommandLineTokens, CommandLineSwitches);

    for (FString SwitchStr : CommandLineSwitches)
    {
        FString Key;
        FString Value;

        if (SwitchStr.Split("=", &Key, &Value))
        {
            if (Key.Equals("port", ESearchCase::IgnoreCase) && Value.IsNumeric())
            {
                LexFromString(ProcessParameters->port, *Value);
                break;
            }
        }
    }
    
    TArray<FString> Logfiles;
    Logfiles.Add(TEXT("DistributedDemo/Saved/Logs/server.log"));
    ProcessParameters->logParameters = Logfiles;
    
    //The game server calls ProcessReady() to tell Amazon GameLift Servers it's ready to host game sessions.
    UE_LOG(LogDedicatedServers, Log, TEXT("Calling Process Ready..."));
    FGameLiftGenericOutcome ProcessReadyOutcome = GameLiftSdkModule->ProcessReady(*ProcessParameters);

    if (ProcessReadyOutcome.IsSuccess())
    {
        UE_LOG(LogDedicatedServers, SetColor, TEXT("%s"), COLOR_GREEN);
        UE_LOG(LogDedicatedServers, Log, TEXT("Process Ready!"));
        UE_LOG(LogDedicatedServers, SetColor, TEXT("%s"), COLOR_NONE);
    }
    else
    {
        UE_LOG(LogDedicatedServers, SetColor, TEXT("%s"), COLOR_RED);
        UE_LOG(LogDedicatedServers, Log, TEXT("ERROR: Process Ready Failed!"));
        FGameLiftError ProcessReadyError = ProcessReadyOutcome.GetError();
        UE_LOG(LogDedicatedServers, Log, TEXT("ERROR: %s"), *ProcessReadyError.m_errorMessage);
        UE_LOG(LogDedicatedServers, SetColor, TEXT("%s"), COLOR_NONE);
    }

    UE_LOG(LogDedicatedServers, Log, TEXT("InitGameLift completed!"));
    bGameLiftInitialized = true;
}
#endif

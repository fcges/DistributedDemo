// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HTTP/HTTPRequestManager.h"
#include "DedicatedServers/DedicatedServers.h"
#include "UI/HTTP/HTTPRequestTypes.h"
#include "JsonObjectConverter.h"
#include "Player/DSLocalPlayerSubsystem.h"

UDSLocalPlayerSubsystem* UHTTPRequestManager::GetLocalPlayerSubsystem() const
{
	APlayerController* LocalPlayerController = GEngine->GetFirstLocalPlayerController(GetWorld());
	if (IsValid(LocalPlayerController))
	{
		ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(LocalPlayerController->Player);
		if (IsValid(LocalPlayer))
		{
			return LocalPlayer->GetSubsystem<UDSLocalPlayerSubsystem>();
		}
	}
	return nullptr;
}

bool UHTTPRequestManager::ContainsErrors(TSharedPtr<FJsonObject> JsonObject)
{
	// Deal with error message in response
	if (JsonObject->HasField(TEXT("errorType")) || JsonObject->HasField(TEXT("errorMessage")))
	{
		FString errorType = JsonObject->HasField(TEXT("errorType")) ? JsonObject->GetStringField(TEXT("errorType")) : TEXT("Unknown error");
		FString ErrorMessage = JsonObject->HasField(TEXT("errorMessage")) ? JsonObject->GetObjectField(TEXT("error"))->GetStringField(TEXT("message")) : TEXT("Unknown error message");
		UE_LOG(LogDedicatedServers, Error, TEXT("Error Type: %s"), *errorType);
		UE_LOG(LogDedicatedServers, Error, TEXT("Error in ListFleets_Response: %s"), *ErrorMessage);
		
		return true;
	}
		
	if (JsonObject->HasField(TEXT("$fault")))
	{
		FString errorType = JsonObject->HasField(TEXT("name")) ? JsonObject->GetStringField(TEXT("name")) : TEXT("Unknown error");
		UE_LOG(LogDedicatedServers, Error, TEXT("Error Type: %s"), *errorType);
		
		return true;
	}
	
	return false;
}

void UHTTPRequestManager::DumpMetaData(TSharedPtr<FJsonObject> JsonObject)
{
	if (JsonObject->HasField(TEXT("$metadata")))
	{
		TSharedPtr<FJsonObject> MetadataJsonObject = JsonObject->GetObjectField(TEXT("$metadata"));
		FDSMetaData DSMetaData;
		FJsonObjectConverter::JsonObjectToUStruct(MetadataJsonObject.ToSharedRef(), &DSMetaData);
			
		DSMetaData.Dump();
	}
}

FString UHTTPRequestManager::SerializeJsonContent(const TMap<FString, FString>& Params)
{
	TSharedPtr<FJsonObject> ContentJsonObject = MakeShareable(new FJsonObject);
	
	for (const auto& Param : Params)
	{
		ContentJsonObject->SetStringField(Param.Key, Param.Value);
	}
	
	FString Content;
	TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&Content);
	FJsonSerializer::Serialize(ContentJsonObject.ToSharedRef(), JsonWriter);
	
	return Content;
}

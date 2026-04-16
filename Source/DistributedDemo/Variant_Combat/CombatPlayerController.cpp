// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Combat/CombatPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "CombatCharacter.h"
#include "Variant_Combat/CombatGameMode.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Blueprint/UserWidget.h"
#include "DistributedDemo.h"
#include "EnhancedInputComponent.h"
#include "Widgets/Input/SVirtualJoystick.h"

void ACombatPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (!IsLocalController())
	{
		return;
	}

	// only spawn touch controls on local player controllers
	if (ShouldUseTouchControls() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogDistributedDemo, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void ACombatPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	EnhancedInputComponent->BindAction(QuitAction, ETriggerEvent::Started, this, &ACombatPlayerController::Input_Quit);

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// add the input mapping context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

void ACombatPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// subscribe to the pawn's OnDestroyed delegate
	InPawn->OnDestroyed.AddDynamic(this, &ACombatPlayerController::OnPawnDestroyed);
}

ACombatPlayerController::ACombatPlayerController()
{
	bQuitMenuOpen = false;
}

void ACombatPlayerController::SetRespawnTransform(const FTransform& NewRespawn)
{
	// save the new respawn transform
	RespawnTransform = NewRespawn;
}

void ACombatPlayerController::EnableInput(APlayerController* PlayerController)
{
	Super::EnableInput(PlayerController);
	
	if (IsValid(GetPawn()) && GetPawn()->Implements<UPlayerInterface>())
	{
		IPlayerInterface::Execute_EnableGameActions(GetPawn(), true);
	}
}

void ACombatPlayerController::DisableInput(APlayerController* PlayerController)
{
	Super::DisableInput(PlayerController);
	
	if (IsValid(GetPawn()) && GetPawn()->Implements<UPlayerInterface>())
	{
		IPlayerInterface::Execute_EnableGameActions(GetPawn(), false);
	}
}

void ACombatPlayerController::OnPawnDestroyed(AActor* DestroyedActor)
{
	// Respawn must be initiated by the server (GameMode) so the new pawn replicates to everyone.
	if (!HasAuthority())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (ACombatGameMode* GM = World->GetAuthGameMode<ACombatGameMode>())
		{
			GM->RequestRespawn(Cast<ACharacter>(DestroyedActor), this);
		}
	}
}

bool ACombatPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

void ACombatPlayerController::Input_Quit()
{
	bQuitMenuOpen = !bQuitMenuOpen;
	if (bQuitMenuOpen)
	{
		FInputModeGameAndUI InputMode;
		SetInputMode(InputMode);
		SetShowMouseCursor(true);
		OnQuitMenuOpen.Broadcast(true);
	}
	else
	{
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		SetShowMouseCursor(false);
		OnQuitMenuOpen.Broadcast(false);
	}
}

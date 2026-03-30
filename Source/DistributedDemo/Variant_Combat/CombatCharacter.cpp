// Copyright Epic Games, Inc. All Rights Reserved.


#include "CombatCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "CombatLifeBar.h"
#include "Engine/DamageEvents.h"
#include "TimerManager.h"
#include "Engine/LocalPlayer.h"
#include "CombatPlayerController.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
// #include "PauseMenuWidget.h"
#include "RPGAttributeSet.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

ACombatCharacter::ACombatCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	// Keep animation notifies and sockets updating on dedicated servers during combat montages.
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	// bind the attack montage ended delegate
	OnAttackMontageEnded.BindUObject(this, &ACombatCharacter::AttackMontageEnded);

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(35.0f, 90.0f);

	// Configure character movement
	GetCharacterMovement()->MaxWalkSpeed = 400.0f;

	// create the camera boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);

	CameraBoom->TargetArmLength = DefaultCameraDistance;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->bEnableCameraRotationLag = true;

	// create the orbiting camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// create the life bar widget component
	LifeBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("LifeBar"));
	LifeBar->SetupAttachment(RootComponent);

	// set the player tag
	Tags.Add(FName("Player"));

	//Attributes

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<URPGAttributeSet>(TEXT("AttributeSet"));
}

UAbilitySystemComponent* ACombatCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ACombatCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void ACombatCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ACombatCharacter::ComboAttackPressed()
{
	// route the input
	DoComboAttackStart();
}

void ACombatCharacter::ChargedAttackPressed()
{
	// route the input
	DoChargedAttackStart();
}

void ACombatCharacter::ChargedAttackReleased()
{
	// route the input
	DoChargedAttackEnd();
}

void ACombatCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void ACombatCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ACombatCharacter::DoComboAttackStart()
{
	if (!HasAuthority())
	{
		ServerDoComboAttackStart();
		return;
	}

	// are we already playing an attack animation?
	if (bIsAttacking)
	{
		// buffer combo input instead of relying only on network-latency-sensitive timestamps.
		const int32 MaxBufferedComboInputs = FMath::Max(1, ComboSectionNames.Num() - 1);
		CachedComboInputCount = FMath::Clamp(CachedComboInputCount + 1, 0, MaxBufferedComboInputs);
		CachedAttackInputTime = GetWorld()->GetTimeSeconds();
		return;
	}

	// perform a combo attack
	ComboAttack();
	MulticastPlayComboAttack();
}

void ACombatCharacter::DoComboAttackEnd()
{
	// stub
}

void ACombatCharacter::DoChargedAttackStart()
{
	if (!HasAuthority())
	{
		ServerDoChargedAttackStart();
		return;
	}

	// raise the charging attack flag
	bIsChargingAttack = true;

	if (bIsAttacking)
	{
		// cache the input time so we can check it later
		CachedAttackInputTime = GetWorld()->GetTimeSeconds();

		return;
	}

	ChargedAttack();
	MulticastPlayChargedAttack();
}

void ACombatCharacter::DoChargedAttackEnd()
{
	if (!HasAuthority())
	{
		ServerDoChargedAttackEnd();
		return;
	}

	// lower the charging attack flag
	bIsChargingAttack = false;

	// if we've done the charge loop at least once, release the charged attack right away
	if (bHasLoopedChargedAttack)
	{
		CheckChargedAttack();
	}
}

void ACombatCharacter::ResetHP()
{
	if (!AttributeSet)
	{
		return;
	}

	AttributeSet->SetCurrentHealth(AttributeSet->GetMaxHealth());
	UpdateLifebar();
}

void ACombatCharacter::ComboAttack()
{
	// raise the attacking flag
	bIsAttacking = true;

	// reset the combo count
	ComboCount = 0;
	CachedComboInputCount = 0;

	// play the attack montage
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		const float MontageLength = AnimInstance->Montage_Play(ComboAttackMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);

		// subscribe to montage completed and interrupted events
		if (MontageLength > 0.0f)
		{
			// set the end delegate for the montage
			AnimInstance->Montage_SetEndDelegate(OnAttackMontageEnded, ComboAttackMontage);
		}
	}

}

void ACombatCharacter::ChargedAttack()
{
	// raise the attacking flag
	bIsAttacking = true;

	// reset the charge loop flag
	bHasLoopedChargedAttack = false;

	// play the charged attack montage
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		const float MontageLength = AnimInstance->Montage_Play(ChargedAttackMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);

		// subscribe to montage completed and interrupted events
		if (MontageLength > 0.0f)
		{
			// set the end delegate for the montage
			AnimInstance->Montage_SetEndDelegate(OnAttackMontageEnded, ChargedAttackMontage);
		}
	}
}

void ACombatCharacter::AttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// reset the attacking flag
	bIsAttacking = false;

	if (Montage == ComboAttackMontage)
	{
		CachedAttackInputTime = 0.0f;
		CachedComboInputCount = 0;
		return;
	}

	// check if we have a non-stale cached input
	if (GetWorld()->GetTimeSeconds() - CachedAttackInputTime <= AttackInputCacheTimeTolerance)
	{
		// are we holding the charged attack button?
		if (bIsChargingAttack)
		{
			// do a charged attack
			ChargedAttack();
			MulticastPlayChargedAttack();
		}
	}

	CachedAttackInputTime = 0.0f;
}

void ACombatCharacter::DoAttackTrace(FName DamageSourceBone)
{
	if (!HasAuthority())
	{
		return;
	}

	// sweep for objects in front of the character to be hit by the attack
	TArray<FHitResult> OutHits;

	// start at the provided socket location, sweep forward
	const FVector TraceStart = GetMesh()->GetSocketLocation(DamageSourceBone);
	const FVector TraceEnd = TraceStart + (GetActorForwardVector() * MeleeTraceDistance);

	// check for pawn and world dynamic collision object types
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	// use a sphere shape for the sweep
	FCollisionShape CollisionShape;
	CollisionShape.SetSphere(MeleeTraceRadius);

	// ignore self
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->SweepMultiByObjectType(OutHits, TraceStart, TraceEnd, FQuat::Identity, ObjectParams, CollisionShape, QueryParams))
	{
		// iterate over each object hit
		for (const FHitResult& CurrentHit : OutHits)
		{
			// check if we've hit a damageable actor
			ICombatDamageable* Damageable = Cast<ICombatDamageable>(CurrentHit.GetActor());

			if (Damageable)
			{
				// knock upwards and away from the impact normal
				const FVector Impulse = (CurrentHit.ImpactNormal * -MeleeKnockbackImpulse) + (FVector::UpVector * MeleeLaunchImpulse);

				// pass the damage event to the actor
				Damageable->ApplyDamage(MeleeDamage, this, CurrentHit.ImpactPoint, Impulse);

				// call the BP handler to play effects, etc.
				DealtDamage(MeleeDamage, CurrentHit.ImpactPoint);
			}
		}
	}
}

void ACombatCharacter::CheckCombo()
{
	if (!HasAuthority())
	{
		return;
	}

	// are we playing a non-charge attack animation?
	if (bIsAttacking && !bIsChargingAttack)
	{
		const bool bHasBufferedComboInput = CachedComboInputCount > 0;
		const bool bHasFreshComboInput = GetWorld()->GetTimeSeconds() - CachedAttackInputTime <= ComboInputCacheTimeTolerance;

		if (bHasBufferedComboInput || bHasFreshComboInput)
		{
			if (bHasBufferedComboInput)
			{
				--CachedComboInputCount;
			}

			// consume the timestamped input when we advance the combo.
			CachedAttackInputTime = 0.0f;

			// increase the combo counter
			++ComboCount;

			// do we still have a combo section to play?
			if (ComboCount < ComboSectionNames.Num())
			{
				// jump to the next combo section
				if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
				{
					AnimInstance->Montage_JumpToSection(ComboSectionNames[ComboCount], ComboAttackMontage);
					MulticastJumpToComboSection(ComboSectionNames[ComboCount]);
				}
			}
		}
	}
}

void ACombatCharacter::CheckChargedAttack()
{
	if (!HasAuthority())
	{
		return;
	}

	// raise the looped charged attack flag
	bHasLoopedChargedAttack = true;

	// jump to either the loop or the attack section depending on whether we're still holding the charge button
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		const FName NextSection = bIsChargingAttack ? ChargeLoopSection : ChargeAttackSection;
		AnimInstance->Montage_JumpToSection(NextSection, ChargedAttackMontage);
		MulticastJumpToChargedSection(NextSection);
	}
}

void ACombatCharacter::ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse)
{
	// pass the damage event to the actor
	FDamageEvent DamageEvent;
	const float ActualDamage = TakeDamage(Damage, DamageEvent, nullptr, DamageCauser);

	// only process knockback and effects if we received nonzero damage
	if (ActualDamage > 0.0f)
	{
		// apply the knockback impulse
		GetCharacterMovement()->AddImpulse(DamageImpulse, true);

		// is the character ragdolling?
		if (GetMesh()->IsSimulatingPhysics())
		{
			// apply an impulse to the ragdoll
			GetMesh()->AddImpulseAtLocation(DamageImpulse * GetMesh()->GetMass(), DamageLocation);
		}

		// pass control to BP to play effects, etc.
		ReceivedDamage(ActualDamage, DamageLocation, DamageImpulse.GetSafeNormal());
	}

}

void ACombatCharacter::SetLevel(float level)
{
	AttributeSet->SetCurrentLevel(level);
}

void ACombatCharacter::SetExp(float exp)
{
	AttributeSet->SetCurrentExp(exp);
}

void ACombatCharacter::UpdateLifebar()
{
	if (!AttributeSet || !LifeBarWidget)
	{
		return;
	}

	const float CurrentHP = AttributeSet->GetCurrentHealth();
	const float MaxHP = AttributeSet->GetMaxHealth();
	const float LifePercent = MaxHP > 0.0f ? CurrentHP / MaxHP : 0.0f;

	UE_LOG(LogTemp, Log, TEXT("HP: %f / %f"), CurrentHP, MaxHP);
	LifeBarWidget->SetLifePercentage(LifePercent);
}

void ACombatCharacter::HandleDeath()
{
	// disable movement while we're dead
	GetCharacterMovement()->DisableMovement();

	// enable full ragdoll physics
	GetMesh()->SetSimulatePhysics(true);

	// hide the life bar
	LifeBar->SetHiddenInGame(true);

	// pull back the camera
	GetCameraBoom()->TargetArmLength = DeathCameraDistance;

	// schedule respawning
	GetWorld()->GetTimerManager().SetTimer(RespawnTimer, this, &ACombatCharacter::RespawnCharacter, RespawnTime, false);
}

void ACombatCharacter::ApplyHealing(float Healing, AActor* Healer)
{
	// stub
}

void ACombatCharacter::RespawnCharacter()
{
	// destroy the character and let it be respawned by the Player Controller
	Destroy();
}

float ACombatCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float CurrentHP = AttributeSet->GetCurrentHealth();
	float MaxHP = AttributeSet->GetMaxHealth();
	if (CurrentHP <= 0.0f) return 0.0f;
	UE_LOG(LogTemp, Log, TEXT("Damage: %f"), Damage);

	CurrentHP -= Damage;
	AttributeSet->SetCurrentHealth(CurrentHP);

	// have we run out of HP?
	if (CurrentHP <= 0.0f)
	{
		// die
		HandleDeath();
	}
	else
	{
		//UpdateLifebar();

		// enable partial ragdoll physics, but keep the pelvis vertical
		GetMesh()->SetPhysicsBlendWeight(0.5f);
		GetMesh()->SetBodySimulatePhysics(PelvisBoneName, false);
	}

	// return the received damage amount
	return Damage;
}

void ACombatCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	float CurrentHP = AttributeSet->GetCurrentHealth();

	// is the character still alive?
	if (CurrentHP >= 0.0f)
	{
		// disable ragdoll physics
		GetMesh()->SetPhysicsBlendWeight(0.0f);
	}
}

void ACombatCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (GetNetMode() != NM_DedicatedServer)
	{
		// get the life bar from the widget component
		LifeBarWidget = Cast<UCombatLifeBar>(LifeBar->GetUserWidgetObject());
		check(LifeBarWidget);
		
		// set the life bar color
		LifeBarWidget->SetBarColor(LifeBarColor);
	}

	// initialize the camera
	GetCameraBoom()->TargetArmLength = DefaultCameraDistance;

	// save the relative transform for the mesh so we can reset the ragdoll later
	MeshStartingTransform = GetMesh()->GetRelativeTransform();

	InitializeAttributes();
	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	// Add currentHealth and maxHealth to HandleHealthChanged
	if (AbilitySystemComponent && AttributeSet)
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetCurrentHealthAttribute()).AddUObject(this, &ACombatCharacter::HandleHealthChanged);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetMaxHealthAttribute()).AddUObject(this, &ACombatCharacter::HandleHealthChanged);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetCurrentLevelAttribute()).AddUObject(this, &ACombatCharacter::HandleLevelChanged);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetCurrentExpAttribute()).AddUObject(this, &ACombatCharacter::HandleExpChanged);
	}

	// Give Abilities
	if (HasAuthority() && AbilitySystemComponent)
	{
		if (DashAbility)
		{
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(DashAbility, 1, 0));
		}

		if (TimeStopAbility)
		{
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(TimeStopAbility, 1, 0));
		}
	}

	// Apply Initial hp
	if (HasAuthority() && AbilitySystemComponent && StatEffect)
	{
		UE_LOG(LogTemp, Log, TEXT("Applying Initial Gameplay Effect at Level %f"), AttributeSet->GetCurrentLevel());
		FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
		FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(
			StatEffect, AttributeSet->GetCurrentLevel(), Context
		);
		if (Spec.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}

	ResetHP();

	// Player HUD
	//if (PlayerHUDClass)
	//{
	//	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	//	{
	//		PlayerHUDWidget = CreateWidget<UPlayerHUDWidget>(PC, PlayerHUDClass);
	//		if (PlayerHUDWidget)
	//		{
	//			PlayerHUDWidget->AddToViewport(0); // low Z-order, behind pause menu
	//			PlayerHUDWidget->UpdateHealthBar(AttributeSet->GetCurrentHealth(), AttributeSet->GetMaxHealth());
	//			PlayerHUDWidget->UpdateExp(AttributeSet->GetCurrentExp(), AttributeSet->GetMaxExp());
	//			PlayerHUDWidget->UpdateLevel((int32)AttributeSet->GetCurrentLevel());
	//		}
	//	}
	//}

	// Optional: pre-create it once
	/*if (PauseMenuClass)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			PauseMenuWidget = CreateWidget<UUserWidget>(PC, PauseMenuClass);
		}
	}

	if (PauseMenuWidget)
	{
		UPauseMenuWidget* TypedWidget = Cast<UPauseMenuWidget>(PauseMenuWidget);
		if (TypedWidget)
		{
			TypedWidget->OnResumeRequested.BindUObject(this, &ACombatCharacter::HidePauseMenu);
		}
	}*/
}

void ACombatCharacter::InitializeAttributes()
{
	if (AbilitySystemComponent && AttributeSet)
	{
		UE_LOG(LogTemp, Log, TEXT("Initializing Attributes"));
	}
}

void ACombatCharacter::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	UpdateLifebar();

	const float DeltaValue = Data.NewValue - Data.OldValue;
	OnHealthChanged(DeltaValue, FGameplayTagContainer());
}

void ACombatCharacter::HandleExpChanged(const FOnAttributeChangeData& Data)
{
	UE_LOG(LogTemp, Log, TEXT("Exp changed: %f / %f"), Data.NewValue, AttributeSet ? AttributeSet->GetMaxExp() : 0.0f);
}

void ACombatCharacter::HandleLevelChanged(const FOnAttributeChangeData& Data)
{
	UE_LOG(LogTemp, Log, TEXT("Level changed: %f -> %f"), Data.OldValue, Data.NewValue);

	if (HasAuthority() && AbilitySystemComponent && StatEffect)
	{
		FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
		FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(
			StatEffect, AttributeSet->GetCurrentLevel(), Context
		);
		if (Spec.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			ResetHP();
		}
	}

	UpdateLifebar();
}

void ACombatCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACombatCharacter, AbilitySystemComponent);
	DOREPLIFETIME(ACombatCharacter, AttributeSet);
	DOREPLIFETIME(ACombatCharacter, bIsAttacking);
	DOREPLIFETIME(ACombatCharacter, bIsChargingAttack);
}

void ACombatCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear the respawn timer
	GetWorld()->GetTimerManager().ClearTimer(RespawnTimer);
}

void ACombatCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACombatCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACombatCharacter::Look);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ACombatCharacter::Look);

		// Combo Attack
		EnhancedInputComponent->BindAction(ComboAttackAction, ETriggerEvent::Started, this, &ACombatCharacter::ComboAttackPressed);

		// Charged Attack
		EnhancedInputComponent->BindAction(ChargedAttackAction, ETriggerEvent::Started, this, &ACombatCharacter::ChargedAttackPressed);
		EnhancedInputComponent->BindAction(ChargedAttackAction, ETriggerEvent::Completed, this, &ACombatCharacter::ChargedAttackReleased);

		//TimeStop Ability
		EnhancedInputComponent->BindAction(TimeStopAction, ETriggerEvent::Started, this, &ACombatCharacter::TimeStopPressed);

		// Pause Menu
		// EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Started, this, &ACombatCharacter::TogglePauseMenu);
	}
}

void ACombatCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

void ACombatCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

void ACombatCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	// update the respawn transform on the Player Controller
	if (ACombatPlayerController* PC = Cast<ACombatPlayerController>(GetController()))
	{
		PC->SetRespawnTransform(GetActorTransform());
	}
}

void ACombatCharacter::TimeStopPressed()
{
	if (!HasAuthority())
	{
		return;
	}

	if (AbilitySystemComponent && TimeStopAbility)
	{
		AbilitySystemComponent->TryActivateAbilityByClass(TimeStopAbility);
	}
}

void ACombatCharacter::TogglePauseMenu()
{
	/*if (bIsPaused)
	{
		HidePauseMenu();
	}
	else
	{
		ShowPauseMenu();
	}*/
}

void ACombatCharacter::ShowPauseMenu()
{
	//if (bIsPaused)
	//{
	//	return;
	//}

	//APlayerController* PC = Cast<APlayerController>(GetController());
	//if (!PC)
	//{
	//	return;
	//}

	//if (!PauseMenuWidget)
	//{
	//	if (!PauseMenuClass)
	//	{
	//		return;
	//	}
	//	PauseMenuWidget = CreateWidget<UUserWidget>(PC, PauseMenuClass);
	//	if (!PauseMenuWidget)
	//	{
	//		return;
	//	}
	//}

	//PauseMenuWidget->AddToViewport(100); // high Z-order
	//bIsPaused = true;

	//// Pause game
	//PC->SetPause(true);

	//// Switch input to UI
	//FInputModeUIOnly InputMode;
	//InputMode.SetWidgetToFocus(PauseMenuWidget->TakeWidget());
	//InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	//PC->SetInputMode(FInputModeGameAndUI());

	//PC->bShowMouseCursor = true;
	//PC->bEnableClickEvents = true;
	//PC->bEnableMouseOverEvents = true;
}

void ACombatCharacter::HidePauseMenu()
{
	//if (!bIsPaused) return;

	//APlayerController* PC = Cast<APlayerController>(GetController());
	//if (!PC)
	//{
	//	return;
	//}

	//if (PauseMenuWidget)
	//{
	//	PauseMenuWidget->RemoveFromParent();
	//}

	//bIsPaused = false;

	//// Unpause
	//PC->SetPause(false);

	//// Restore game input
	//PC->SetInputMode(FInputModeGameOnly());

	//PC->bShowMouseCursor = false;
	//PC->bEnableClickEvents = false;
	//PC->bEnableMouseOverEvents = false;
}

void ACombatCharacter::NotifyDanger(const FVector& DangerLocation, AActor* DangerSource)
{
	return;
}

void ACombatCharacter::ServerDoComboAttackStart_Implementation()
{
	DoComboAttackStart();
}

void ACombatCharacter::ServerDoChargedAttackStart_Implementation()
{
	DoChargedAttackStart();
}

void ACombatCharacter::ServerDoChargedAttackEnd_Implementation()
{
	DoChargedAttackEnd();
}

void ACombatCharacter::MulticastPlayComboAttack_Implementation()
{
	if (HasAuthority())
	{
		return;
	}

	ComboAttack();
}

void ACombatCharacter::MulticastPlayChargedAttack_Implementation()
{
	if (HasAuthority())
	{
		return;
	}

	ChargedAttack();
}

void ACombatCharacter::MulticastJumpToComboSection_Implementation(FName SectionName)
{
	if (HasAuthority())
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_JumpToSection(SectionName, ComboAttackMontage);
	}
}

void ACombatCharacter::MulticastJumpToChargedSection_Implementation(FName SectionName)
{
	if (HasAuthority())
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_JumpToSection(SectionName, ChargedAttackMontage);
	}
}

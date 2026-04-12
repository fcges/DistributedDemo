// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/StatusComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "RPGAttributeSet.h"
#include "GameFramework/Actor.h"


// Sets default values for this component's properties
UStatusComponent::UStatusComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UStatusComponent::BeginPlay()
{
	Super::BeginPlay();

	TryInitializeFromOwner();
	BindAttributeDelegates();
	
}

void UStatusComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindAttributeDelegates();
	CachedASC = nullptr;
	CachedAttributeSet = nullptr;

	Super::EndPlay(EndPlayReason);
}

float UStatusComponent::GetHealth() const
{
	return CachedAttributeSet ? CachedAttributeSet->GetCurrentHealth() : 0.0f;
}

float UStatusComponent::GetMaxHealth() const
{
	return CachedAttributeSet ? CachedAttributeSet->GetMaxHealth() : 0.0f;
}

float UStatusComponent::GetHealthNormalized() const
{
	const float MaxVal = GetMaxHealth();
	return (MaxVal > 0.0f) ? (GetHealth() / MaxVal) : 0.0f;
}

void UStatusComponent::TryInitializeFromOwner()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Prefer AbilitySystemInterface.
	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Owner))
	{
		CachedASC = ASI->GetAbilitySystemComponent();
	}

	// Fallback: attempt to find an ASC component directly.
	if (!CachedASC)
	{
		CachedASC = Owner->FindComponentByClass<UAbilitySystemComponent>();
	}

	// Resolve the attribute set from ASC (preferred) or from known owner types.
	CachedAttributeSet = nullptr;
	if (CachedASC)
	{
		for (UAttributeSet* Set : CachedASC->GetSpawnedAttributes())
		{
			if (URPGAttributeSet* RPGSet = Cast<URPGAttributeSet>(Set))
			{
				CachedAttributeSet = RPGSet;
				break;
			}
		}
	}
	if (!CachedAttributeSet)
	{
		// Fallback: if the attribute set was created as a named default subobject on the owner, try locating it by name.
		// This avoids needing to access protected members on specific character classes.
		CachedAttributeSet = FindObject<URPGAttributeSet>(Owner, TEXT("AttributeSet"));
	}
}

void UStatusComponent::BindAttributeDelegates()
{
	if (!CachedASC || !CachedAttributeSet)
	{
		return;
	}

	UnbindAttributeDelegates();

	CurrentHealthChangedHandle = CachedASC
		->GetGameplayAttributeValueChangeDelegate(CachedAttributeSet->GetCurrentHealthAttribute())
		.AddUObject(this, &UStatusComponent::HandleCurrentHealthChanged);

	MaxHealthChangedHandle = CachedASC
		->GetGameplayAttributeValueChangeDelegate(CachedAttributeSet->GetMaxHealthAttribute())
		.AddUObject(this, &UStatusComponent::HandleMaxHealthChanged);

	// Push an initial value so UI has correct data immediately after bind.
	OnHealthChanged.Broadcast(this, GetHealth(), GetHealth(), nullptr);
	OnMaxHealthChanged.Broadcast(this, GetMaxHealth(), GetMaxHealth(), nullptr);
}

void UStatusComponent::UnbindAttributeDelegates()
{
	if (!CachedASC || !CachedAttributeSet)
	{
		CurrentHealthChangedHandle.Reset();
		MaxHealthChangedHandle.Reset();
		return;
	}

	if (CurrentHealthChangedHandle.IsValid())
	{
		CachedASC->GetGameplayAttributeValueChangeDelegate(CachedAttributeSet->GetCurrentHealthAttribute())
			.Remove(CurrentHealthChangedHandle);
		CurrentHealthChangedHandle.Reset();
	}

	if (MaxHealthChangedHandle.IsValid())
	{
		CachedASC->GetGameplayAttributeValueChangeDelegate(CachedAttributeSet->GetMaxHealthAttribute())
			.Remove(MaxHealthChangedHandle);
		MaxHealthChangedHandle.Reset();
	}
}

void UStatusComponent::HandleCurrentHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	OnHealthChanged.Broadcast(this, ChangeData.OldValue, ChangeData.NewValue, nullptr);
}

void UStatusComponent::HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	OnMaxHealthChanged.Broadcast(this, ChangeData.OldValue, ChangeData.NewValue, nullptr);
}


// Called every frame
void UStatusComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                     FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// If we were created before ASC was ready (or owner changed), attempt lazy init.
	if (!CachedASC || !CachedAttributeSet)
	{
		TryInitializeFromOwner();
		BindAttributeDelegates();
	}
}


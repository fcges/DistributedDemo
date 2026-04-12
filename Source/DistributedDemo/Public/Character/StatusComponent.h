// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "StatusComponent.generated.h"


class UAbilitySystemComponent;
class URPGAttributeSet;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FStatus_AttributeChanged, UStatusComponent*, StatusComponent, float, OldValue, float, NewValue, AActor*, Instigator);


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DISTRIBUTEDDEMO_API UStatusComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UStatusComponent();

	/** Returns the status component if one exists on the specified actor. (UI compatibility helper) */
	UFUNCTION(BlueprintPure, Category = "DistributedDemo|Status")
	static UStatusComponent* FindStatusComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<UStatusComponent>() : nullptr); }

	/** Current health (from GAS attribute set). */
	UFUNCTION(BlueprintCallable, Category = "DistributedDemo|Status")
	float GetHealth() const;

	/** Current max health (from GAS attribute set). */
	UFUNCTION(BlueprintCallable, Category = "DistributedDemo|Status")
	float GetMaxHealth() const;

	/** Current health in range [0,1]. */
	UFUNCTION(BlueprintCallable, Category = "DistributedDemo|Status")
	float GetHealthNormalized() const;

	/** Delegate fired when health changes. Instigator may be null on clients. */
	UPROPERTY(BlueprintAssignable, Category = "DistributedDemo|Status")
	FStatus_AttributeChanged OnHealthChanged;

	/** Delegate fired when max health changes. Instigator may be null on clients. */
	UPROPERTY(BlueprintAssignable, Category = "DistributedDemo|Status")
	FStatus_AttributeChanged OnMaxHealthChanged;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Resolve and bind to the owning actor's ASC/AttributeSet (if available). */
	void TryInitializeFromOwner();
	void BindAttributeDelegates();
	void UnbindAttributeDelegates();

	void HandleCurrentHealthChanged(const FOnAttributeChangeData& ChangeData);
	void HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData);

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UAbilitySystemComponent> CachedASC;

	UPROPERTY(Transient)
	TObjectPtr<URPGAttributeSet> CachedAttributeSet;

	FDelegateHandle CurrentHealthChangedHandle;
	FDelegateHandle MaxHealthChangedHandle;
};

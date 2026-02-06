// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "CA_HealingDrone_Alt.generated.h"

class AHealingShieldActor;

/**
 * 
 */

UCLASS()
class AURA_API UCA_HealingDrone_Alt : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UCA_HealingDrone_Alt();

protected:
	// 보호막 Actor (SphereCollision / 역장)
	UPROPERTY(EditDefaultsOnly, Category="HealingDrone|Spawn")
	TSubclassOf<AHealingShieldActor> HealingShieldActorClass;

	// 쉴드 스폰 후 플레이어에 붙일지 여부
	UPROPERTY(EditDefaultsOnly, Category="HealingDrone|Shield")
	bool bAttachShieldToOwner = true;

	// Attach 할 때 소켓/본 (없으면 NAME_None)
	UPROPERTY(EditDefaultsOnly, Category="HealingDrone|Shield")
	FName ShieldAttachSocketName = NAME_None;

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

private:
	// 스폰된 보호막
	UPROPERTY()
	TObjectPtr<AHealingShieldActor> SpawnedShieldActor = nullptr;

	UFUNCTION()
	void OnShieldDestroyed(AActor* DestroyedActor);

	void CleanupSpawnedShield();
	
};

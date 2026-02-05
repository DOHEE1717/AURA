// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "CA_OrbitalStrike_Alt.generated.h"

/**
 * 
 */
class AGameplayAbilityTargetActor;
struct FGameplayAbilityTargetDataHandle;
struct FGameplayEventData;

UCLASS()
class AURA_API UCA_OrbitalStrike_Alt : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UCA_OrbitalStrike_Alt();

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

protected:
	// ===== Targeting =====
	UPROPERTY(EditDefaultsOnly, Category="OrbitalStrike|Alt|Targeting")
	TSubclassOf<AGameplayAbilityTargetActor> TargetActorClass;

	// Recon 반경
	UPROPERTY(EditDefaultsOnly, Category="OrbitalStrike|Alt|Tuning")
	float ReconRadius = 1200.f;

	// 전투 판정 태그(ASC에 이 태그가 있으면 bCombatMode=true)
	UPROPERTY(EditDefaultsOnly, Category="OrbitalStrike|Alt|Tags")
	FGameplayTag CombatStateTag;

	// Targeting 입력 라우팅 태그
	UPROPERTY(EditDefaultsOnly, Category="OrbitalStrike|Alt|Tags")
	FGameplayTag TargetingActiveTag;

private:
	UFUNCTION()
	void OnTargetDataReady(const FGameplayAbilityTargetDataHandle& DataHandle);

	UFUNCTION()
	void OnTargetDataCancelled(const FGameplayAbilityTargetDataHandle& DataHandle);

	void CleanupTargetingTag();
	
};

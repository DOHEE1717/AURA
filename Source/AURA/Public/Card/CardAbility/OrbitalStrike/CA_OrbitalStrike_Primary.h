// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "CA_OrbitalStrike_Primary.generated.h"


class AGameplayAbilityTargetActor;
class UAbilityTask_WaitTargetData;
class AOrbitalStrikeActor;


/**
 *
 * 
 */
UCLASS()
class AURA_API UCA_OrbitalStrike_Primary : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UCA_OrbitalStrike_Primary();

protected:
	// ====== UGameplayAbility ======
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;

protected:
	// ====== Targeting Callbacks ======
	UFUNCTION()
	void OnTargetDataReady(const FGameplayAbilityTargetDataHandle& DataHandle);

	UFUNCTION()
	void OnTargetDataCancelled(const FGameplayAbilityTargetDataHandle& DataHandle);

	void LogTargetData(const FGameplayAbilityTargetDataHandle& DataHandle) const;

protected:
	// 2클릭 확정 방식 (GravityField와 동일)
	UPROPERTY(EditDefaultsOnly, Category="OrbitalStrike|Targeting")
	TEnumAsByte<EGameplayTargetingConfirmation::Type> ConfirmationType;

	// 추후 BP_TA_OrbitalStrike 지정
	UPROPERTY(EditDefaultsOnly, Category="OrbitalStrike|Targeting")
	TSubclassOf<AGameplayAbilityTargetActor> TargetActorClass;

	// 추후 OrbitalStrikeActor(레이저 지속딜) 스폰용
	UPROPERTY(EditDefaultsOnly, Category="OrbitalStrike|Actor")
	TSubclassOf<AOrbitalStrikeActor> OrbitalStrikeActorClass;

private:
	UPROPERTY()
	TObjectPtr<AGameplayAbilityTargetActor> SpawnedTargetActor = nullptr;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitTargetData> WaitTargetDataTask = nullptr;
	
};

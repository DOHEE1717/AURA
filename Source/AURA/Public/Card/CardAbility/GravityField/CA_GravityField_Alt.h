// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "CA_GravityField_Alt.generated.h"

class AGameplayAbilityTargetActor;
class UAbilityTask_WaitTargetData;
class AGravityFieldReverseActor;

/**
 * 
 */
UCLASS()
class AURA_API UCA_GravityField_Alt : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UCA_GravityField_Alt();

protected:
	// TargetActor 지정 (Primary와 동일)
	UPROPERTY(EditDefaultsOnly, Category="Targeting")
	TSubclassOf<AGameplayAbilityTargetActor> TargetActorClass;

	// 타겟 확정 방식 (Primary와 동일)
	UPROPERTY(EditDefaultsOnly, Category="Targeting")
	TEnumAsByte<EGameplayTargetingConfirmation::Type> ConfirmationType;

	// Alt 효과 Actor (역중력)
	UPROPERTY(EditDefaultsOnly, Category="Effect")
	TSubclassOf<AGravityFieldReverseActor> GravityFieldReverseActorClass;
		

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
	// 스폰한 TargetActor(데칼 프리뷰)
	UPROPERTY()
	TObjectPtr<AGameplayAbilityTargetActor> SpawnedTargetActor = nullptr;

	// WaitTargetData Task (Valid/Cancelled 콜백 받는 녀석)
	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitTargetData> WaitTargetDataTask = nullptr;

	UFUNCTION()
	void OnTargetDataReady(const FGameplayAbilityTargetDataHandle& DataHandle);

	UFUNCTION()
	void OnTargetDataCancelled(const FGameplayAbilityTargetDataHandle& DataHandle);

	// Primary처럼 로그가 필요하면 사용 (선택)
	void LogTargetData(const FGameplayAbilityTargetDataHandle& DataHandle) const;
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "CA_GravityField_Primary.generated.h"

class AGameplayAbilityTargetActor;
class UAbilityTask_WaitTargetData;
class AGravityFieldActor;


/**
 * 
 */

//1클릭 : TargetActor 로 decal 프리뷰
//2클릭 : TargetData 처리(예정) 

UCLASS()
class AURA_API UCA_GravityField_Primary : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UCA_GravityField_Primary();
	
protected:
	//TargetActor 지정
	UPROPERTY(EditDefaultsOnly,Category="Targeting")
	TSubclassOf<AGameplayAbilityTargetActor> TargetActorClass;
	
	//타겟확정방식
	UPROPERTY(EditDefaultsOnly,Category="Targeting")
	TEnumAsByte<EGameplayTargetingConfirmation::Type> ConfirmationType;
	
	UPROPERTY(EditDefaultsOnly, Category="Effect")
	TSubclassOf<AGravityFieldActor> GravityFieldActorClass;
	
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
	
	void LogTargetData(const FGameplayAbilityTargetDataHandle& DataHandle) const;
	
	
};

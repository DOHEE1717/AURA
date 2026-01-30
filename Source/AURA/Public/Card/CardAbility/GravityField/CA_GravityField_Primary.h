// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "CA_GravityField_Primary.generated.h"

class AGameplayAbilityTargetActor;
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
	
protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle, 
		const FGameplayAbilityActorInfo* ActorInfo, 
		const FGameplayAbilityActivationInfo ActivationInfo, 
		const FGameplayEventData* TriggerEventData) override;
	
	UFUNCTION()
	void OnTargetDataReady(const FGameplayAbilityTargetDataHandle& DataHandle);
	
	UFUNCTION()
	void OnTargetDataCancelled(const FGameplayAbilityTargetDataHandle& DataHandle);
	
	void LogTargetData(const FGameplayAbilityTargetDataHandle& DataHandle) const;
	
	
};

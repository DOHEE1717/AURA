// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "CardAbility_Fireball.generated.h"

class AFireballProjectile;
/**
 * 
 */
UCLASS()
class AURA_API UCardAbility_Fireball : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UCardAbility_Fireball();
	
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActiveInfo, const FGameplayEventData* TriggerEventData) override;
	
protected:
	//스폰할 클래스
	UPROPERTY(EditDefaultsOnly, Category="CardAbility|Fireball")
	TSubclassOf<AFireballProjectile> FireballClass;
	
	// 스폰 위치 오프셋(앞/위로 조금)
	UPROPERTY(EditDefaultsOnly, Category="CardAbility|Fireball")
	FVector SpawnOffset = FVector(150.f, 0.f, 50.f);
};

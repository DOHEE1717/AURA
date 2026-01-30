// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "AuraPlayerState.generated.h"


class UAbilitySystemComponent;
class UAuraCombatCardComponent;
class UDA_AuraCardAbilityMapping;
class UGameplayAbility;


/**
 * 
 */
UCLASS()
class AURA_API AAuraPlayerState : public APlayerState,public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:
	AAuraPlayerState();
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	UFUNCTION(BlueprintCallable,Category="GAS")
	UAbilitySystemComponent* GetASC() const {return AbilitySystemComponent;}
	
	/** 전투 카드 컴포넌트 Getter */
	UFUNCTION(BlueprintCallable, Category="Aura|CombatCard")
	UAuraCombatCardComponent* GetCombatCardComponent() const { return CombatCardComponent; }
	
protected:
	//GAS를 구현하는 ASC(AbilitySystemComponent) 컴포넌트 생성
	UPROPERTY(VisibleAnywhere,blueprintReadOnly,Category="GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	// Combat Card Component 가져오기
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Aura|CombatCard")
	TObjectPtr<UAuraCombatCardComponent> CombatCardComponent;
	
protected:
	virtual void BeginPlay() override;

private:
	void GrantCombatCardAbilities();
	
};

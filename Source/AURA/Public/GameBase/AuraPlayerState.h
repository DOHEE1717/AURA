// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "AuraPlayerState.generated.h"


class UAbilitySystemComponent;
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
	
protected:
	//GAS를 구현하는 ASC(AbilitySystemComponent) 컴포넌트 생성
	UPROPERTY(VisibleAnywhere,blueprintReadOnly,Category="GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
};

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
class UOrbitalReconComponent;
class UAuraAttributeSet;
class UPhaseShiftRecallComponent;

/**
 * Aura PlayerState (ASC + AttributeSet + CombatCard)
 */
UCLASS()
class AURA_API AAuraPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AAuraPlayerState();

	/** IAbilitySystemInterface */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintCallable, Category="GAS")
	UAbilitySystemComponent* GetASC() const { return AbilitySystemComponent; }

	UFUNCTION(BlueprintCallable, Category="GAS")
	UAuraAttributeSet* GetAttributeSet() const { return AttributeSet; }

	/** 전투 카드 컴포넌트 Getter */
	UFUNCTION(BlueprintCallable, Category="Aura|CombatCard")
	UAuraCombatCardComponent* GetCombatCardComponent() const { return CombatCardComponent; }

	UFUNCTION(BlueprintCallable, Category="Aura|Recon")
	UOrbitalReconComponent* GetOrbitalReconComponent() const { return OrbitalReconComp; }
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Aura|PhaseShift")
	TObjectPtr<UPhaseShiftRecallComponent> PhaseShiftRecallComp;

public:
	UFUNCTION(BlueprintCallable, Category="Aura|PhaseShift")
	UPhaseShiftRecallComponent* GetPhaseShiftRecallComponent() const { return PhaseShiftRecallComp; }

protected:
	/** GAS ASC */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	/** 기본 AttributeSet (MoveSpeed 포함) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GAS")
	TObjectPtr<UAuraAttributeSet> AttributeSet;

	/** Combat Card Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Aura|CombatCard")
	TObjectPtr<UAuraCombatCardComponent> CombatCardComponent;

	/** Orbital Recon Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Aura|Recon")
	TObjectPtr<UOrbitalReconComponent> OrbitalReconComp;

protected:
	virtual void BeginPlay() override;

private:
	void GrantCombatCardAbilities();
};

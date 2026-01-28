// Fill out your copyright notice in the Description page of Project Settings.


#include "GameBase/AuraPlayerState.h"
#include "AbilitySystemComponent.h"
#include "Card/AuraCombatCardComponent.h"



AAuraPlayerState::AAuraPlayerState()
{
	bReplicates=true;
	
	AbilitySystemComponent=CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	
	//CardCombatComponent 추가
	CombatCardComponent=CreateDefaultSubobject<UAuraCombatCardComponent>(TEXT("CombatCardComponent"));
	
}

UAbilitySystemComponent* AAuraPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

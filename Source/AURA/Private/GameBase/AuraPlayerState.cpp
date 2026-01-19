// Fill out your copyright notice in the Description page of Project Settings.


#include "GameBase/AuraPlayerState.h"
#include "AbilitySystemComponent.h"



AAuraPlayerState::AAuraPlayerState()
{
	bReplicates=true;
	
	AbilitySystemComponent=CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	
}

UAbilitySystemComponent* AAuraPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

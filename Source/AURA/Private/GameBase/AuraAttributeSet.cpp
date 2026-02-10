// Fill out your copyright notice in the Description page of Project Settings.


#include "GameBase/AuraAttributeSet.h"

#include "Net/UnrealNetwork.h"

UAuraAttributeSet::UAuraAttributeSet()
{
	// 기본 이동속도 (Character 기본값과 맞춤)
	MoveSpeed.SetBaseValue(600.f);
	MoveSpeed.SetCurrentValue(600.f);
}

void UAuraAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, MoveSpeed, OldValue);
}

void UAuraAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(
		UAuraAttributeSet,
		MoveSpeed,
		COND_None,
		REPNOTIFY_Always
	);
}
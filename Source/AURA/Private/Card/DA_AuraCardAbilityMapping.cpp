// Fill out your copyright notice in the Description page of Project Settings.


#include "Card/DA_AuraCardAbilityMapping.h"

bool UDA_AuraCardAbilityMapping::GetAbilityPair(const FName CardID, FCardAbilityPair& OutPair) const
{
	if (const FCardAbilityPair* Found = AbilityMap.Find(CardID))
	{
		OutPair = *Found;
		return true;
	}
	return false;
}
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DA_AuraCardAbilityMapping.generated.h"

class UGameplayAbility;
/**
 * 
 */

//카드 한장당 좌클릭(pri),우클릭(Alt) 를 매핑 
USTRUCT(BlueprintType)
struct FCardAbilityPair 
{
	GENERATED_BODY()
	
public:
	//좌클릭(primary)
	UPROPERTY(EditAnywhere, BlueprintReadOnly,Category="Card Ability")
	TSubclassOf<UGameplayAbility> PrimaryAbility;
	
	//우클릭(Alt)
	UPROPERTY(EditAnywhere, BlueprintReadOnly,Category="Card Ability")
	TSubclassOf<UGameplayAbility> AltAbility;
	
};


//카드 ID를 받아서 매핑

UCLASS(BlueprintType)
class AURA_API UDA_AuraCardAbilityMapping : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	//카드 ID(FName) -> CardAbilityPair
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card Ability")
	TMap<FName, FCardAbilityPair> AbilityMap;

public:
	//없으면 null 반환해서 crash 방지
	UFUNCTION(BlueprintCallable, Category="Card Ability")
	bool GetAbilityPair(const FName CardID, FCardAbilityPair& OutPair) const;
	
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Card/CardEnum.h"
#include "DA_CardDefinition.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UDA_CardDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category=Card)
	FName CardID=NAME_None;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Card")
	ECardType CardType = ECardType::None;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Card")
	int32 CardCost = 0;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Card|UI")
	FText CardName;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Card|UI", meta=(MultiLine="true"))
	FText CardDescription;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Card|UI")
	TObjectPtr<UTexture2D> CardIcon = nullptr;
	
};

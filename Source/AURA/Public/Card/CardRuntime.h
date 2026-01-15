// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Card/CardEnum.h"
#include "CardRuntime.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FCardRuntime
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="Card")
	FName CardID=NAME_None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="Card")
	ECardType CardType=ECardType::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="Card")
	int32 CardCost=0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="Card")
	FText CardName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="Card")
	FText CardDescription;
	
	
};
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CardEnum.generated.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class ECardType:uint8
{
	None	UMETA(DisplayName="None"),
	Attack UMETA(DisplayName="Attack"),
	Type UMETA(DisplayName="Type"),
	Modifier UMETA(DisplayName="Modifier"),
};


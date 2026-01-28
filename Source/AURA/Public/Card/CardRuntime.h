// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Card/CardEnum.h"
#include "CardRuntime.generated.h"

/**
 * 
 */

//카드 기본 데이터 
USTRUCT(BlueprintType)
struct FCardRuntime
{
	GENERATED_BODY()
	
	//카드 ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="Card")
	FName CardID=NAME_None;
	
	//카드타입
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="Card")
	ECardType CardType=ECardType::None;
	
	//코스트
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="Card")
	int32 CardCost=0;
	
	//카드이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="Card")
	FText CardName;
	
	//카드설명
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="Card")
	FText CardDescription;
		
};

//카드 재생산 런타임 데이터
USTRUCT(BlueprintType)
struct FCombatCardReproRuntime
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card|Combat")
	FName CardID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card|Combat")
	ECombatCardReproState State = ECombatCardReproState::Ready;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card|Combat")
	float RemainingTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card|Combat")
	float TotalTime = 0.f;

	float GetProgress01() const
	{
		if (TotalTime <= 0.f) return 1.f;
		return FMath::Clamp(1.f - (RemainingTime / TotalTime), 0.f, 1.f);
	}
};
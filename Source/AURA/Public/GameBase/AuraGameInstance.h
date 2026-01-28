// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"

#include "Card/CardRuntime.h"
#include "Card/DA_CardDefinition.h"


#include "AuraGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UAuraGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	virtual  void Init() override;
	
	UFUNCTION(BlueprintCallable, Category="Card")
	void BuildCardRegistry();
	
public:
	UPROPERTY(BlueprintReadWrite,Category="Card|Runtime")
	TArray<FCardRuntime> Deck;
	
	UPROPERTY(BlueprintReadWrite, Category="Card|Runtime")
	TArray<FCardRuntime> Hand;
	
	
	UPROPERTY(BlueprintReadWrite, Category="Card|Runtime")
	TArray<FCardRuntime> Discard;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Card|Runtime")
	int32 MaxHandSize = 3;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Card|Runtime")
	int32 Battery = 3;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Card|Definition")
	TMap<FName, TObjectPtr<UDA_CardDefinition>> CardRegistry;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card|Definition")
	TArray<TObjectPtr<UDA_CardDefinition>> AllCardDefinitions;
	
	
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PlayerController_Aura.generated.h"

/**
 * 
 */
class UInputMappingContext;
class UInputAction;

UCLASS()
class AURA_API APlayerController_Aura : public APlayerController
{
	GENERATED_BODY()
	
public:
	APlayerController_Aura();
	
protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	
private:
	//IA들 붙이기
	
	//IMC 카드
	UPROPERTY(EditDefaultsOnly, Category="Input|Card")
	TObjectPtr<UInputMappingContext>IMC_Card;
	
	//좌클릭
	UPROPERTY(EditDefaultsOnly, Category="Input|Card")
	TObjectPtr<UInputAction> IA_CardLMB;
	
	//우클릭
	UPROPERTY(EditDefaultsOnly, Category="Input|Card")
	TObjectPtr<UInputAction> IA_CardRMB;
	
	
};

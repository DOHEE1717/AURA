#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AuraPlayerController.generated.h"

/**
 * 
 */
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

UCLASS()
class AURA_API AAuraPlayerController: public APlayerController
{
	GENERATED_BODY()
	
public:
	AAuraPlayerController();
	
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
	
	//카드넘기기(휠)
	UPROPERTY(EditDefaultsOnly, Category="Input|Card")
	TObjectPtr<UInputAction> IA_CardSelect;
	
	UPROPERTY(VisibleAnywhere, Category="Card")
	int32 SelectedIndex = 0; // 0~2
	
private:
	//연결함수
	void OnCardLMB();
	void OnCardRMB();
	void OnCardSelect(const FInputActionValue& Value);
	void SelectNext(); //>다음 카드
	void SelectPrev(); //<이전 카드
	
	
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "GameBase/AuraGameplayTags.h"
#include "Blueprint/UserWidget.h"

#include "AuraPlayerController.generated.h"


/**
 * 
 */
class UInputMappingContext;
class UInputAction;
class UAuraCombatCardComponent;
class UAuraCombatHUDWidget;
class UOrbitalReconComponent;
class UUserWidget;
class AOrbitalReconActor;
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
	
protected:
	// BP에서 WBP_Combat 지정
	UPROPERTY(EditDefaultsOnly, Category="UI|Combat")
	TSubclassOf<UUserWidget> CombatHUDClass;

	UPROPERTY()
	TObjectPtr<UAuraCombatHUDWidget> CombatHUDWidget;
	
private:
	//IA들 붙이기
	
	//IMC 카드
	UPROPERTY()
	TObjectPtr<UInputMappingContext> IMC_Card;

	UPROPERTY()
	TObjectPtr<UInputAction> IA_CardLMB;

	UPROPERTY()
	TObjectPtr<UInputAction> IA_CardRMB;

	UPROPERTY()
	TObjectPtr<UInputAction> IA_CardSelect;
	
	// ===== Recon Input =====
	UPROPERTY()
	TObjectPtr<UInputMappingContext> IMC_Recon;

	UPROPERTY()
	TObjectPtr<UInputAction> IA_ReconMove;

	UPROPERTY()
	TObjectPtr<UInputAction> IA_ReconZoom;

	UPROPERTY()
	TObjectPtr<UInputAction> IA_ReconExit;
	
	UPROPERTY(VisibleAnywhere, Category="Card")
	int32 SelectedIndex = 0; // 0~2
	
		

	
private:
	//연결함수
	void OnCardLMB();
	void OnCardRMB();
	void OnCardSelect(const FInputActionValue& Value);
	void SelectNext(); //>다음 카드
	void SelectPrev(); //<이전 카드
	void TryUseSelectedSlotCard(bool bAltClick);
		
	// Recon Handlers 
	bool IsReconViewActive() const;
	void OnReconMove(const FInputActionValue& Value);
	void OnReconZoom(const FInputActionValue& Value);
	void OnReconExit();
	
};

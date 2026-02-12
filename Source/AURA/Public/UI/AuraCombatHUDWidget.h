#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/AuraCombatHUDTypes.h"
#include "AuraCombatHUDWidget.generated.h"

class UTextBlock;
// class UScrollBox;
class UAuraCombatCardComponent;

UCLASS()
class AURA_API UAuraCombatHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// PlayerController에서 CombatCardComponent 연결
	UFUNCTION(BlueprintCallable)
	void BindCombatComponent(UAuraCombatCardComponent* InComponent);

protected:
	/* ===== Slot Text ===== */
	UPROPERTY(meta=(BindWidget)) UTextBlock* TXT_Slot1_CardID;
	UPROPERTY(meta=(BindWidget)) UTextBlock* TXT_Slot2_CardID;
	UPROPERTY(meta=(BindWidget)) UTextBlock* TXT_Slot3_CardID;

	// /* ===== Reproduction List ===== */
	// UPROPERTY(meta=(BindWidget)) UScrollBox* SB_ReproList;

protected:
	/* ===== Delegate Callbacks ===== */
	UFUNCTION()
	void OnSlotsChanged(const TArray<FCombatSlotUIData>& Slots);

	UFUNCTION()
	void OnReprosChanged(const TArray<FCombatReproUIData>& Repros);
	
public:
	UFUNCTION(BlueprintCallable, Category="Aura|CombatHUD")
	void SetSelectedSlotIndex(int32 InIndex);

	// UFUNCTION()
	// void OnNextChanged(FName NextCardID);

protected:
	UPROPERTY()
	TObjectPtr<UAuraCombatCardComponent> BoundComponent;

	void RebuildReproList(const TArray<FCombatReproUIData>& Repros);
};

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/AuraCombatHUDTypes.h"
#include "AuraCombatHUDWidget.generated.h"

class UAuraCombatCardComponent;
class UBorder;
class USizeBox;
class UCanvasPanel;
class UAuraCombatCardSlotWidget;

UCLASS()
class AURA_API UAuraCombatHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void BindCombatComponent(UAuraCombatCardComponent* InComponent);

protected:
	virtual void NativeConstruct() override;

	/* ===== Slot Borders (WBP 이름과 동일해야 함) ===== */
	//사이즈 박스로 수정
    UPROPERTY(meta=(BindWidget)) USizeBox* Slot_1;
    UPROPERTY(meta=(BindWidget)) USizeBox* Slot_2;
    UPROPERTY(meta=(BindWidget)) USizeBox* Slot_3;

	UPROPERTY(meta=(BindWidget))
	UCanvasPanel* CP_Slots;

	/* ===== Runtime Cached Slot Widgets ===== */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UAuraCombatCardSlotWidget>> SlotWidgets;

	/* ===== Delegate Callbacks ===== */
	UFUNCTION()
	void OnSlotsChanged(const TArray<FCombatSlotUIData>& Slots);

	UFUNCTION()
	void OnReprosChanged(const TArray<FCombatReproUIData>& Repros);

public:
	UFUNCTION(BlueprintCallable, Category="Aura|CombatHUD")
	void SetSelectedSlotIndex(int32 InIndex);

protected:
	UPROPERTY()
	TObjectPtr<UAuraCombatCardComponent> BoundComponent;

	void RebuildReproList(const TArray<FCombatReproUIData>& Repros);
};
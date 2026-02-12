#pragma once
#include "CoreMinimal.h"
#include "AuraCombatHUDTypes.generated.h"

// 슬롯 1칸 표시용
USTRUCT(BlueprintType)
struct FCombatSlotUIData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FName CardID = NAME_None;
	UPROPERTY(BlueprintReadOnly) FString StateText;     // "IN_SLOT", "EMPTY" 같은 텍스트
	UPROPERTY(BlueprintReadOnly) float Remaining = 0.f; // 남은시간(필요시)
	UPROPERTY(BlueprintReadOnly) float Duration = 0.f;  // 총시간(필요시)
};

// 재생산 트랙 한 줄 표시용
USTRUCT(BlueprintType)
struct FCombatReproUIData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FName CardID = NAME_None;
	UPROPERTY(BlueprintReadOnly) float Remaining = 0.f;
	UPROPERTY(BlueprintReadOnly) float Duration = 0.f;
	UPROPERTY(BlueprintReadOnly) bool bReady = false;
};

// ===== Delegates =====
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatSlotsUIChanged, const TArray<FCombatSlotUIData>&, Slots);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatReproUIChanged, const TArray<FCombatReproUIData>&, Repros);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatNextUIChanged, FName, NextCardID);

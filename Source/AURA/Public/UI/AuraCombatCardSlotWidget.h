#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AuraCombatCardSlotWidget.generated.h"

class UImage;
class UTextBlock;

UCLASS()
class AURA_API UAuraCombatCardSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 카드 상단 아트(텍스처) 세팅
	UFUNCTION(BlueprintCallable, Category="CombatCardSlot")
	void SetCardArt(UTexture2D* InTexture);

	// 카드 이름 세팅
	UFUNCTION(BlueprintCallable, Category="CombatCardSlot")
	void SetCardName(const FText& InName);

	// 재생산 남은 시간(초). 0 이하이면 숨김
	UFUNCTION(BlueprintCallable, Category="CombatCardSlot")
	void SetReproTimeSeconds(float RemainingSeconds, int32 DecimalPlaces = 0);

protected:
	// WBP_CombatCardSlot 내부 위젯 이름과 1:1로 맞춰야 함
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> IMG_Art;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TXT_Name;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TXT_ReproTime;
};
// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/AuraCombatCardSlotWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

void UAuraCombatCardSlotWidget::SetCardArt(UTexture2D* InTexture)
{
	if (!IMG_Art) return;

	if (InTexture)
	{
		IMG_Art->SetBrushFromTexture(InTexture, true);
		IMG_Art->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		// 텍스처 없으면 숨김(원하는 정책으로 바꿔도 됨)
		IMG_Art->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UAuraCombatCardSlotWidget::SetCardName(const FText& InName)
{
	if (!TXT_Name) return;
	TXT_Name->SetText(InName);
}

static FString FormatSeconds(float Seconds, int32 DecimalPlaces)
{
	Seconds = FMath::Max(0.f, Seconds);

	DecimalPlaces = FMath::Clamp(DecimalPlaces, 0, 2);

	if (DecimalPlaces <= 0)
	{
		const int32 IntSec = FMath::CeilToInt(Seconds);
		return FString::FromInt(IntSec);
	}

	return FString::Printf(TEXT("%.*f"), DecimalPlaces, Seconds);
}

void UAuraCombatCardSlotWidget::SetReproTimeSeconds(float RemainingSeconds, int32 DecimalPlaces)
{
	if (!TXT_ReproTime) return;
	
	DecimalPlaces = 0;

	if (RemainingSeconds <= 0.f)
	{
		TXT_ReproTime->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	TXT_ReproTime->SetVisibility(ESlateVisibility::HitTestInvisible);
	TXT_ReproTime->SetText(FText::FromString(FormatSeconds(RemainingSeconds, DecimalPlaces)));
}
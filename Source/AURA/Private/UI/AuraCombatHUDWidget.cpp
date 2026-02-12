#include "UI/AuraCombatHUDWidget.h"
#include "Card/AuraCombatCardComponent.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Components/Border.h"

static FText MakeSlotText(const FCombatSlotUIData& D)
{
	if (D.CardID.IsNone())
	{
		return FText::FromString(TEXT("EMPTY"));
	}

	return FText::FromString(
		FString::Printf(TEXT("%s [%s]"),
			*D.CardID.ToString(),
			*D.StateText));
}

void UAuraCombatHUDWidget::BindCombatComponent(UAuraCombatCardComponent* InComponent)
{
	if (!InComponent)
	{
		return;
	}

	if (BoundComponent)
	{
		BoundComponent->OnCombatSlotsUIChanged.RemoveAll(this);
		BoundComponent->OnCombatReproUIChanged.RemoveAll(this);
	}

	BoundComponent = InComponent;

	BoundComponent->OnCombatSlotsUIChanged.AddDynamic(this, &ThisClass::OnSlotsChanged);
	BoundComponent->OnCombatReproUIChanged.AddDynamic(this, &ThisClass::OnReprosChanged);

	// 최초 1회 강제 동기화
	BoundComponent->BroadcastUI_All();
}

void UAuraCombatHUDWidget::OnSlotsChanged(const TArray<FCombatSlotUIData>& Slots)
{
	if (Slots.Num() < 3)
	{
		return;
	}

	if (TXT_Slot1_CardID)
	{
		TXT_Slot1_CardID->SetText(MakeSlotText(Slots[0]));
	}

	if (TXT_Slot2_CardID)
	{
		TXT_Slot2_CardID->SetText(MakeSlotText(Slots[1]));
	}

	if (TXT_Slot3_CardID)
	{
		TXT_Slot3_CardID->SetText(MakeSlotText(Slots[2]));
	}
}

void UAuraCombatHUDWidget::OnReprosChanged(const TArray<FCombatReproUIData>& Repros)
{
	RebuildReproList(Repros);
}

void UAuraCombatHUDWidget::SetSelectedSlotIndex(int32 InIndex)
{
	const int32 Sel = FMath::Clamp(InIndex, 0, 2);

	UBorder* B1 = Cast<UBorder>(GetWidgetFromName(TEXT("Slot_1")));
	UBorder* B2 = Cast<UBorder>(GetWidgetFromName(TEXT("Slot_2")));
	UBorder* B3 = Cast<UBorder>(GetWidgetFromName(TEXT("Slot_3")));

	if (!B1 || !B2 || !B3)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HUD] SetSelectedSlotIndex: Slot_1/2/3 Border not found or not Border"));
		return;
	}

	auto Apply = [](UBorder* B, bool bSelected)
	{
		if (!B) return;

		// 선택: 불투명 / 비선택: 반투명
		FLinearColor C = B->GetBrushColor();
		C.A = bSelected ? 1.0f : 0.35f;
		B->SetBrushColor(C);

		// 선택: 패딩 조금 키워서 “굵게” 보이게 (테두리 두께가 따로 없을 때 대체)
		B->SetPadding(bSelected ? FMargin(4.f) : FMargin(1.f));
	};

	Apply(B1, Sel == 0);
	Apply(B2, Sel == 1);
	Apply(B3, Sel == 2);
}

void UAuraCombatHUDWidget::RebuildReproList(const TArray<FCombatReproUIData>& Repros)
{
	// if (!SB_ReproList)
	// {
	// 	return;
	// }
	//
	// SB_ReproList->ClearChildren();
	//
	// for (const FCombatReproUIData& D : Repros)
	// {
	// 	UTextBlock* Line = NewObject<UTextBlock>(SB_ReproList);
	//
	// 	const FString S = FString::Printf(
	// 		TEXT("%s : %.1f / %.1f"),
	// 		*D.CardID.ToString(),
	// 		D.Remaining,
	// 		D.Duration
	// 	);
	//
	// 	Line->SetText(FText::FromString(S));
	// 	SB_ReproList->AddChild(Line);
	// }
}

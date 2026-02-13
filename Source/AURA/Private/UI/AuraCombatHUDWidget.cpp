#include "UI/AuraCombatHUDWidget.h"
#include "Card/AuraCombatCardComponent.h"
#include "UI/AuraCombatCardSlotWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/GameInstance.h"
#include "GameBase/AuraGameInstance.h"
#include "Card/DA_CardDefinition.h"
#include "Engine/Texture2D.h"
#include "Components/SizeBox.h"




static const UDA_CardDefinition* FindCardDefFromRegistry(UWorld* World, const FName CardID)
{
	if (!World || CardID.IsNone())
		return nullptr;

	const UGameInstance* GIBase = World->GetGameInstance();
	const UAuraGameInstance* GI = Cast<UAuraGameInstance>(GIBase);
	if (!GI)
		return nullptr;

	// ValueType이 TObjectPtr<UDA_CardDefinition> 인 케이스
	const TObjectPtr<UDA_CardDefinition>* Found = GI->CardRegistry.Find(CardID);
	return Found ? Found->Get() : nullptr;
}


void UAuraCombatHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SlotWidgets.Empty();
	SlotWidgets.SetNum(3);

	auto GetSlotWidgetFromSizeBox = [](USizeBox* Box) -> UAuraCombatCardSlotWidget*
	{
		if (!Box) return nullptr;

		// USizeBox는 단일 Child 컨테이너
		UWidget* Child = Box->GetContent();
		return Cast<UAuraCombatCardSlotWidget>(Child);
	};

	SlotWidgets[0] = GetSlotWidgetFromSizeBox(Slot_1);
	SlotWidgets[1] = GetSlotWidgetFromSizeBox(Slot_2);
	SlotWidgets[2] = GetSlotWidgetFromSizeBox(Slot_3);

	UE_LOG(LogTemp, Warning, TEXT("[HUD] SlotWidget Cache: %s %s %s"),
		SlotWidgets[0] ? TEXT("OK") : TEXT("NULL"),
		SlotWidgets[1] ? TEXT("OK") : TEXT("NULL"),
		SlotWidgets[2] ? TEXT("OK") : TEXT("NULL"));
}

void UAuraCombatHUDWidget::BindCombatComponent(UAuraCombatCardComponent* InComponent)
{
	if (!InComponent)
		return;

	if (BoundComponent)
	{
		BoundComponent->OnCombatSlotsUIChanged.RemoveAll(this);
		BoundComponent->OnCombatReproUIChanged.RemoveAll(this);
	}

	BoundComponent = InComponent;

	BoundComponent->OnCombatSlotsUIChanged.AddDynamic(this, &ThisClass::OnSlotsChanged);
	BoundComponent->OnCombatReproUIChanged.AddDynamic(this, &ThisClass::OnReprosChanged);

	BoundComponent->BroadcastUI_All();
}

void UAuraCombatHUDWidget::OnSlotsChanged(const TArray<FCombatSlotUIData>& Slots)
{
	if (Slots.Num() < 3)
		return;

	for (int32 i = 0; i < 3; ++i)
	{
		if (!SlotWidgets.IsValidIndex(i) || !SlotWidgets[i])
			continue;

		const FCombatSlotUIData& D = Slots[i];

		// ===== 1) 이름 =====
		const FText NameText = D.CardID.IsNone()
			? FText::FromString(TEXT("EMPTY"))
			: FText::FromName(D.CardID);

		SlotWidgets[i]->SetCardName(NameText);

		// ===== 2) DA 조회(1회) + 아이콘/코스트 =====
		const UDA_CardDefinition* Def = nullptr;
		UTexture2D* IconTex = nullptr;
		int32 CostSeconds = 0;

		if (!D.CardID.IsNone())
		{
			Def = FindCardDefFromRegistry(GetWorld(), D.CardID);
			if (Def)
			{
				IconTex = Def->CardIcon;     // TObjectPtr<UTexture2D> -> UTexture2D*
				CostSeconds = Def->CardCost; // 재생산 시간(초)로 사용
			}
		}

		UE_LOG(LogTemp, Warning, TEXT("[HUD] Slot%d CardID=%s | Def=%s | Icon=%s | Cost=%d"),
			i,
			*D.CardID.ToString(),
			Def ? TEXT("FOUND") : TEXT("NULL"),
			IconTex ? TEXT("OK") : TEXT("NULL"),
			CostSeconds);

		// ===== 3) 슬롯 위젯 반영 =====
		SlotWidgets[i]->SetCardArt(IconTex);

		// 좌상단 숫자: 코스트(초) 표시
		// (SlotWidget 내부가 float/ int 둘 다 쓰는 구조라면 둘 다 넣어줌)
		SlotWidgets[i]->SetReproTimeSeconds((float)CostSeconds, CostSeconds);
	}
}

void UAuraCombatHUDWidget::OnReprosChanged(const TArray<FCombatReproUIData>& Repros)
{
	RebuildReproList(Repros);
}

void UAuraCombatHUDWidget::SetSelectedSlotIndex(int32 InIndex)
{
	const int32 Sel = FMath::Clamp(InIndex, 0, 2);

	auto GetBaseAngle = [](int32 Index) -> float
	{
		if (Index == 0) return -10.f;
		if (Index == 2) return  10.f;
		return 0.f;
	};

	auto ApplyFanStyle = [&](USizeBox* Box, bool bSelected, int32 Index)
	{
		if (!Box) return;

		// 기본 스케일 + 선택 시 더 큼(살짝 줄인 버전)
		const float BaseScale = 0.85f;
		const float SelectedScale =0.95;

		const float Angle = GetBaseAngle(Index);

		// 팬 느낌: 비선택 살짝 아래, 선택은 위로
		const float BaseY = 4.f;
		const float SelectedY = -18.f;

		const float Scale = bSelected ? SelectedScale : BaseScale;
		const float Y = bSelected ? SelectedY : BaseY;

		Box->SetRenderScale(FVector2D(Scale, Scale));
		Box->SetRenderTransformAngle(Angle);
		Box->SetRenderTranslation(FVector2D(0.f, Y));
	};

	ApplyFanStyle(Slot_1, Sel == 0, 0);
	ApplyFanStyle(Slot_2, Sel == 1, 1);
	ApplyFanStyle(Slot_3, Sel == 2, 2);

	// ===== 선택 카드 최상단(ZOrder) 올리기 =====
	auto SetZ = [](UWidget* W, int32 Z)
	{
		if (!W) return;

		// Slot_1/2/3는 CP_Slots(Canvas Panel) 아래에 있어야 CanvasPanelSlot로 캐스팅 가능
		if (UCanvasPanelSlot* CS = Cast<UCanvasPanelSlot>(W->Slot))
		{
			CS->SetZOrder(Z);
		}
	};

	SetZ(Slot_1, (Sel == 0) ? 10 : 0);
	SetZ(Slot_2, (Sel == 1) ? 10 : 0);
	SetZ(Slot_3, (Sel == 2) ? 10 : 0);
}

void UAuraCombatHUDWidget::RebuildReproList(const TArray<FCombatReproUIData>& Repros)
{
	// 현재 재생산 리스트는 사용 안 함
}






























































































































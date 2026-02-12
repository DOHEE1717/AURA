// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/AuraCombatReproTrackWidget.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Blueprint/WidgetTree.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

#include "GameBase/AuraPlayerState.h"
#include "Card/AuraCombatCardComponent.h"

#include "GameBase/AuraGameInstance.h"
#include "Card/DA_CardDefinition.h"

#include "UObject/UnrealType.h"

void UAuraCombatReproTrackWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindToCombatComponent();

	// 초기 1회 강제 동기화(컴포넌트가 이미 데이터를 갖고 있을 수 있음)
	if (CardComp)
	{
		CardComp->BroadcastUI_All();
	}
}

void UAuraCombatReproTrackWidget::NativeDestruct()
{
	if (CardComp)
	{
		CardComp->OnCombatReproUIChanged.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UAuraCombatReproTrackWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 떨어지는 위치는 매 프레임 갱신
	UpdateLayout(MyGeometry);
}

void UAuraCombatReproTrackWidget::BindToCombatComponent()
{
	CardComp = ResolveCombatCardComponent();
	if (!CardComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ReproTrack] CombatCardComponent not found"));
		return;
	}

	CardComp->OnCombatReproUIChanged.AddDynamic(this, &ThisClass::HandleReproChanged);
}

UAuraCombatCardComponent* UAuraCombatReproTrackWidget::ResolveCombatCardComponent() const
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC) return nullptr;

	APlayerState* PS = PC->PlayerState;
	if (!PS) return nullptr;

	if (AAuraPlayerState* AuraPS = Cast<AAuraPlayerState>(PS))
	{
		return AuraPS->GetCombatCardComponent();
	}

	// 혹시 다른 PS 구조면 여기서 확장
	return nullptr;
}

void UAuraCombatReproTrackWidget::HandleReproChanged(const TArray<FCombatReproUIData>& InRepros)
{
	// 1) 이전 Active 목록 저장
	TSet<FName> PrevActive;
	for (const auto& Pair : ActiveRepros)
	{
		PrevActive.Add(Pair.Key);
	}

	// 2) 새 Active 구성
	ActiveRepros.Reset();

	TSet<FName> NewActive;
	for (const FCombatReproUIData& D : InRepros)
	{
		if (D.CardID.IsNone()) continue;

		ActiveRepros.Add(D.CardID, D);
		NewActive.Add(D.CardID);

		// 재생산 아이콘 확보
		GetOrCreateReproIcon(D.CardID);
	}

	// 3) Active에서 빠진 애들은 "완료"로 간주하고 Ready 스택으로 이동
	for (const FName& WasActive : PrevActive)
	{
		if (NewActive.Contains(WasActive)) continue;

		// Repro 아이콘 제거
		RemoveReproIcon(WasActive);

		// Ready 스택에 추가(중복 방지)
		if (!ReadyStackOrder.Contains(WasActive))
		{
			ReadyStackOrder.Add(WasActive);
		}

		// Ready 아이콘 확보
		GetOrCreateReadyIcon(WasActive);
	}

	// 4) 레이아웃 즉시 1회 갱신(틱에서도 계속 갱신됨)
	if (Canvas_Track)
	{
		const FGeometry& G = GetCachedGeometry();
		UpdateLayout(G);
	}
}

UUserWidget* UAuraCombatReproTrackWidget::GetOrCreateReproIcon(FName CardID)
{
	if (TObjectPtr<UUserWidget>* Found = ReproIcons.Find(CardID))
	{
		return Found->Get();
	}

	if (!Canvas_Track || !IconWidgetClass) return nullptr;

	UUserWidget* W = CreateWidget<UUserWidget>(GetOwningPlayer(), IconWidgetClass);
	if (!W) return nullptr;

	Canvas_Track->AddChild(W);

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(W->Slot))
	{
		CanvasSlot->SetAutoSize(false);
		CanvasSlot->SetSize(FVector2D(IconSize, IconSize));
		CanvasSlot->SetAlignment(FVector2D(0.f, 0.f));
	}

	ReproIcons.Add(CardID, W);
	ApplyIconVisual(W, CardID, /*bIsReady=*/false);
	return W;
}

UUserWidget* UAuraCombatReproTrackWidget::GetOrCreateReadyIcon(FName CardID)
{
	// ✅ ReadyIcons에서 찾는다
	if (TObjectPtr<UUserWidget>* Found = ReadyIcons.Find(CardID))
	{
		return Found->Get();
	}

	if (!Canvas_Track || !IconWidgetClass) return nullptr;

	UUserWidget* W = CreateWidget<UUserWidget>(GetOwningPlayer(), IconWidgetClass);
	if (!W) return nullptr;

	Canvas_Track->AddChild(W);

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(W->Slot))
	{
		CanvasSlot->SetAutoSize(false);
		CanvasSlot->SetSize(FVector2D(IconSize, IconSize));
		CanvasSlot->SetAlignment(FVector2D(0.f, 0.f));
	}

	ReadyIcons.Add(CardID, W);
	ApplyIconVisual(W, CardID, /*bIsReady=*/true);
	return W;
}

void UAuraCombatReproTrackWidget::RemoveReproIcon(FName CardID)
{
	if (TObjectPtr<UUserWidget>* Found = ReproIcons.Find(CardID))
	{
		if (UUserWidget* W = Found->Get())
		{
			W->RemoveFromParent();
		}
		ReproIcons.Remove(CardID);
	}
}

void UAuraCombatReproTrackWidget::ApplyIconVisual(UUserWidget* IconWidget, FName CardID, bool bIsReady)
{
	if (!IconWidget) return;

	// 아이콘 위젯 내부에 Img_Icon이라는 UImage가 있다고 가정
	UWidget* W = IconWidget->WidgetTree ? IconWidget->WidgetTree->FindWidget(TEXT("Img_Icon")) : nullptr;
	UImage* Img = Cast<UImage>(W);
	if (!Img) return;

	// Tint
	Img->SetColorAndOpacity(bIsReady ? ReadyTint : ReproTint);

	// Card Icon Texture 적용 (있으면)
	if (UTexture2D* Tex = FindCardIconTexture(CardID))
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Tex);
		Brush.ImageSize = FVector2D(IconSize, IconSize);
		Img->SetBrush(Brush);
	}
}

UTexture2D* UAuraCombatReproTrackWidget::FindCardIconTexture(FName CardID) const
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	UAuraGameInstance* GI = World->GetGameInstance<UAuraGameInstance>();
	if (!GI) return nullptr;

	// CardRegistry의 ValueType이 프로젝트마다 다를 수 있으니 auto로 안전 처리
	const auto* Found = GI->CardRegistry.Find(CardID);
	if (!Found) return nullptr;

	// 1) CardRegistry 값에서 UDA_CardDefinition을 최대한 안전하게 뽑는다
	const UDA_CardDefinition* Def = nullptr;

	// (A) ValueType이 UObject* 계열이면
	if constexpr (TIsPointer<std::remove_reference_t<decltype(*Found)>>::Value)
	{
		Def = Cast<UDA_CardDefinition>(*Found);
	}
	else
	{
		// (B) ValueType이 TObjectPtr<...> 계열이면
		Def = Cast<UDA_CardDefinition>(Found->Get());
	}

	if (!Def) return nullptr;

	// 2) Def 안에서 "CardIcon" 프로퍼티를 리플렉션으로 찾는다
	static const FName PropName(TEXT("CardIcon"));
	FProperty* P = Def->GetClass()->FindPropertyByName(PropName);
	if (!P) return nullptr;

	FObjectProperty* ObjProp = CastField<FObjectProperty>(P);
	if (!ObjProp) return nullptr;

	UObject* Obj = ObjProp->GetObjectPropertyValue_InContainer(Def);
	return Cast<UTexture2D>(Obj);
}

float UAuraCombatReproTrackWidget::GetProgress01(const FCombatReproUIData& D) const
{
	if (D.Duration <= 0.f) return 1.f;
	return FMath::Clamp(1.f - (D.Remaining / D.Duration), 0.f, 1.f);
}

float UAuraCombatReproTrackWidget::GetBottomTargetY(float TrackHeight) const
{
	// 완료 스택이 이미 N개 있으면, 다음 완성은 그 위에 "도착"시키기
	const float BottomY = FMath::Max(0.f, TrackHeight - IconSize);
	const float Reserve = (IconSize + StackGap) * ReadyStackOrder.Num();
	return FMath::Max(0.f, BottomY - Reserve);
}

void UAuraCombatReproTrackWidget::UpdateLayout(const FGeometry& MyGeometry)
{
	if (!Canvas_Track) return;

	const FVector2D Size = MyGeometry.GetLocalSize();
	const float TrackWidth = Size.X;
	const float TrackHeight = Size.Y;

	UpdateReadyStackLayout(TrackWidth, TrackHeight);
	UpdateReproLayout(TrackWidth, TrackHeight);
}

void UAuraCombatReproTrackWidget::UpdateReadyStackLayout(float TrackWidth, float TrackHeight)
{
	// 아래부터 차곡차곡 쌓기(완료 순서대로)
	const float X = FMath::Max(0.f, (TrackWidth - IconSize) * 0.5f);
	const float BottomY = FMath::Max(0.f, TrackHeight - IconSize);

	for (int32 i = 0; i < ReadyStackOrder.Num(); ++i)
	{
		const FName CardID = ReadyStackOrder[i];
		UUserWidget* Icon = nullptr;

		if (TObjectPtr<UUserWidget>* Found = ReadyIcons.Find(CardID))
		{
			Icon = Found->Get();
		}

		if (!Icon) continue;

		const float Y = BottomY - (IconSize + StackGap) * i;

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Icon->Slot))
		{
			CanvasSlot->SetPosition(FVector2D(X, Y));
		}

		// 혹시 중간에 틴트가 바뀌었을 수 있으니 보장
		ApplyIconVisual(Icon, CardID, /*bIsReady=*/true);
	}
}

void UAuraCombatReproTrackWidget::UpdateReproLayout(float TrackWidth, float TrackHeight)
{
	const float X = FMath::Max(0.f, (TrackWidth - IconSize) * 0.5f);

	const float TopY = 0.f;
	const float BottomTargetY = GetBottomTargetY(TrackHeight);

	for (const auto& Pair : ActiveRepros)
	{
		const FName CardID = Pair.Key;
		const FCombatReproUIData& D = Pair.Value;

		UUserWidget* Icon = nullptr;
		if (TObjectPtr<UUserWidget>* Found = ReproIcons.Find(CardID))
		{
			Icon = Found->Get();
		}
		if (!Icon) continue;

		const float P01 = GetProgress01(D);
		const float Y = FMath::Lerp(TopY, BottomTargetY, P01);

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Icon->Slot))
		{
			CanvasSlot->SetPosition(FVector2D(X, Y));
		}

		ApplyIconVisual(Icon, CardID, /*bIsReady=*/false);
	}
}
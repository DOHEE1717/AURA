// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/AuraCombatHUDTypes.h"
#include "AuraCombatReproTrackWidget.generated.h"

class UCanvasPanel;
class UAuraCombatCardComponent;

UCLASS()
class AURA_API UAuraCombatReproTrackWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	// WBP_CombatReproTrack 안에 존재해야 함 (이름 정확히 "Canvas_Track")
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCanvasPanel> Canvas_Track;

	// WBP에서 기본값으로 WBP_CombatReproIcon 지정해둘 것
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aura|Repro")
	TSubclassOf<UUserWidget> IconWidgetClass;

	// 아이콘 크기(정사각)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aura|Repro")
	float IconSize = 64.f;

	// 완료 스택 간격
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aura|Repro")
	float StackGap = 6.f;

	// 재생산 중(회색) Tint
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aura|Repro")
	FLinearColor ReproTint = FLinearColor(0.55f, 0.55f, 0.55f, 1.f);

	// 완료(밝게) Tint
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aura|Repro")
	FLinearColor ReadyTint = FLinearColor(1.f, 1.f, 1.f, 1.f);

private:
	// CombatCardComponent 연결
	UPROPERTY()
	TObjectPtr<UAuraCombatCardComponent> CardComp;

	// 현재 재생산 중 데이터 캐시 (CardID -> UIData)
	TMap<FName, FCombatReproUIData> ActiveRepros;

	// 재생산 중 표시용 아이콘 (CardID -> Widget)
	UPROPERTY()
	TMap<FName, TObjectPtr<UUserWidget>> ReproIcons;

	// 완료 스택 순서(완료된 순서대로 아래부터 쌓임)
	UPROPERTY()
	TArray<FName> ReadyStackOrder;

	// 완료 스택 아이콘 (CardID -> Widget)
	UPROPERTY()
	TMap<FName, TObjectPtr<UUserWidget>> ReadyIcons;

	// Delegate 수신
	UFUNCTION()
	void HandleReproChanged(const TArray<FCombatReproUIData>& InRepros);

	// 유틸
	void BindToCombatComponent();
	UAuraCombatCardComponent* ResolveCombatCardComponent() const;

	// Icon 관리
	UUserWidget* GetOrCreateReproIcon(FName CardID);
	UUserWidget* GetOrCreateReadyIcon(FName CardID);
	void RemoveReproIcon(FName CardID);

	// 아이콘 이미지/틴트 적용
	void ApplyIconVisual(UUserWidget* IconWidget, FName CardID, bool bIsReady);

	// CardID -> CardIcon(텍스처) 추출 (리플렉션)
	UTexture2D* FindCardIconTexture(FName CardID) const;

	// Canvas 배치 갱신
	void UpdateLayout(const FGeometry& MyGeometry);
	void UpdateReadyStackLayout(float TrackWidth, float TrackHeight);
	void UpdateReproLayout(float TrackWidth, float TrackHeight);

	float GetProgress01(const FCombatReproUIData& D) const;
	float GetBottomTargetY(float TrackHeight) const;
};

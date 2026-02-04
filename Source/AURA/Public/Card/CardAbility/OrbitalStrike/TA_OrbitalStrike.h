// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "TA_OrbitalStrike.generated.h"


class UDecalComponent;
class UMaterialInterface;

/**
 * 
 */
UCLASS()
class AURA_API ATA_OrbitalStrike : public AGameplayAbilityTargetActor
{
	GENERATED_BODY()
	
public:
	ATA_OrbitalStrike();

	// AGameplayAbilityTargetActor
	virtual void StartTargeting(UGameplayAbility* Ability) override;
	virtual void Tick(float DeltaTime) override;

	// 2번째 클릭(Confirm)
	virtual void ConfirmTargetingAndContinue() override;

	// 취소(Cancel)
	virtual void CancelTargeting() override;

protected:
	// 데칼 프리뷰
	UPROPERTY(VisibleAnywhere, Category="OrbitalStrike")
	TObjectPtr<UDecalComponent> PreviewDecal;

	// 데칼 머티리얼 (BP에서 지정해도 됨)
	UPROPERTY(EditDefaultsOnly, Category="OrbitalStrike")
	TObjectPtr<UMaterialInterface> PreviewDecalMaterial;

	// 범위(프리뷰 크기)
	UPROPERTY(EditDefaultsOnly, Category="OrbitalStrike")
	float Radius = 300.0f;

	// 트레이스 거리
	UPROPERTY(EditDefaultsOnly, Category="OrbitalStrike")
	float TraceDistance = 3000.f;

	// 바닥 위치 캐시
	FHitResult CachedHitResult;

	// 조준 컨트롤러 캐시
	TWeakObjectPtr<APlayerController> CachedPC;

	// 프레임마다 바닥 위치 갱신
	void UpdateAimHitResult();

	// 프레임마다 데칼 갱신
	void UpdatePreviewDecal();
};

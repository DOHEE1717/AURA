// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "TA_GravityField.generated.h"

class UDecalComponent;
class UMaterialInterface;
/**
 * 
 */
UCLASS()
class AURA_API ATA_GravityField : public AGameplayAbilityTargetActor
{
	GENERATED_BODY()
	
public:
	ATA_GravityField();
	
	//AGameplayAbilityTargetActor 인터페이스
	virtual void StartTargeting(UGameplayAbility* Ability) override;
	virtual void Tick(float DeltaTime) override;
	
	//2번쨰 클릭시 연결할 함수
	virtual void ConfirmTargetingAndContinue() override;
	
	//취소 처리 구현 함수
	virtual void CancelTargeting() override;
	
protected:
	//decal 프리뷰
	UPROPERTY(VisibleAnywhere,Category="GravityField")
	TObjectPtr<UDecalComponent> PreviewDecal;
	
	//decal 머터리얼
	UPROPERTY(EditDefaultsOnly,Category="GravityField")
	TObjectPtr<UMaterialInterface> PreviewDecalMaterial;
	
	//범위 
	UPROPERTY(EditDefaultsOnly,Category="GravityField")
	float Radius = 400.0f;
	
	//거리
	UPROPERTY(EditDefaultsOnly,Category="GravityField")
	float TraceDistance=3000.f;
	
	//바닥위치 캐시
	FHitResult CachedHitResult;
	
	//조준 컨트롤러 캐시 
	TWeakObjectPtr<APlayerController> CachedPC;
	
	//프레임마다 바닥 위치 갱신
	void UpdateAimHitResult();
	
	//프레임마다 decal 갱신
	void UpdatePreviewDecal();
	
	
	
};

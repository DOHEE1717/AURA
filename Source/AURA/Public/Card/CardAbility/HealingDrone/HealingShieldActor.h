// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HealingShieldActor.generated.h"

class USphereComponent;

/**
 * Sphere 기반 역장(보호막)
 * - 외부 데미지를 누적(ApplyDamage) 받아 HP가 0이 되면 Destroy
 * - 충돌/피격 판정은 “현재는 최소 구현”: Sphere는 Overlap만 제공
 */
UCLASS()
class AURA_API AHealingShieldActor : public AActor
{
	GENERATED_BODY()

public:
	AHealingShieldActor();

protected:
	virtual void BeginPlay() override;

public:
	// ===== Components =====
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Shield")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Shield")
	TObjectPtr<USphereComponent> ShieldSphere;

	// ===== Shield Stats =====
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Shield|Stat")
	float MaxHP = 100.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Shield|Stat")
	float CurrentHP = 0.f;

	// 보호막 반경
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Shield|Stat")
	float Radius = 120.f;

	// 디버그 표시
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Shield|Debug")
	bool bDrawDebug = true;

	// ===== API =====

	// 외부에서 호출: 데미지 적용 (0 이하 데미지는 무시)
	UFUNCTION(BlueprintCallable, Category="Shield")
	void ApplyDamage(float DamageAmount);

	// (선택) 즉시 파괴용
	UFUNCTION(BlueprintCallable, Category="Shield")
	void BreakShield();

	// (선택) 현재 HP 비율
	UFUNCTION(BlueprintPure, Category="Shield")
	float GetHPRatio() const;

private:
	void SyncSphereRadius();
	void Die();
};

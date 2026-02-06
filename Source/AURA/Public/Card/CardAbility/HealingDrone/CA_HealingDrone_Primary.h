// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "CA_HealingDrone_Primary.generated.h"


class AHealingDroneActor;
/**
 * 좌클릭(Primary): 드론 스폰 + 플레이어 힐 레이저(디버그 라인)
 * - 타겟팅 없음 (즉시 발동)
 * - HealDuration 후 종료(드론 파괴 + EndAbility)
 */

UCLASS()
class AURA_API UCA_HealingDrone_Primary : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UCA_HealingDrone_Primary();

protected:
	// 드론 Actor (큐브 드론)
	UPROPERTY(EditDefaultsOnly, Category="HealingDrone|Spawn")
	TSubclassOf<AHealingDroneActor> HealingDroneActorClass;

	// 드론 스폰 오프셋 (플레이어 기준)
	UPROPERTY(EditDefaultsOnly, Category="HealingDrone|Spawn")
	FVector DroneSpawnOffset = FVector(60.f, 40.f, 120.f);

	// 지속 시간(초): 시간 기반 종료
	UPROPERTY(EditDefaultsOnly, Category="HealingDrone|Primary")
	float HealDuration = 5.f;

	// 레이저 갱신 주기(초): DebugLine 깜빡임/갱신용
	UPROPERTY(EditDefaultsOnly, Category="HealingDrone|Primary")
	float TraceInterval = 0.05f;

	// Trace 길이(혹시 나중에 확장용)
	UPROPERTY(EditDefaultsOnly, Category="HealingDrone|Primary")
	float TraceDistance = 2000.f;

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

private:
	// 스폰된 드론
	UPROPERTY()
	TObjectPtr<AHealingDroneActor> SpawnedDroneActor = nullptr;

	FTimerHandle TimerHandle_TickTrace;
	FTimerHandle TimerHandle_EndByDuration;

	// 주기적으로 “힐 레이저” 느낌 디버그라인
	void TickHealTrace();

	// HealDuration 만료 종료
	void OnHealDurationFinished();

	void CleanupSpawnedDrone();
};

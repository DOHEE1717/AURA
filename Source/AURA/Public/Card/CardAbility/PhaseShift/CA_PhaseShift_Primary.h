// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "CA_PhaseShift_Primary.generated.h"

class UAbilityTask_WaitDelay;
class UGameplayEffect;
class UCharacterMovementComponent;
class ACharacter;

UCLASS()
class AURA_API UCA_PhaseShift_Primary : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UCA_PhaseShift_Primary();

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

protected:
	UFUNCTION()
	void OnBuffFinished();
	
private:
	FTimerHandle DebugSpeedTimerHandle;

	UFUNCTION()
	void DebugLogSpeedTick();

protected:
	// ===== 설정값(2번 스펙: 이속 + FOV) =====
	UPROPERTY(EditDefaultsOnly, Category="PhaseShift|Config")
	float BuffDuration = 3.0f;

	// 로컬 시각 효과(FOV)
	UPROPERTY(EditDefaultsOnly, Category="PhaseShift|Config")
	float FOVDelta = 10.0f;

	// 폴백(직접 MaxWalkSpeed 조정) 배율
	UPROPERTY(EditDefaultsOnly, Category="PhaseShift|Config")
	float MoveSpeedMultiplierFallback = 1.5f;

	// GE(정석 이동속도 적용)
	UPROPERTY(EditDefaultsOnly, Category="PhaseShift|Config")
	TSubclassOf<UGameplayEffect> MoveSpeedGameplayEffectClass;

	// Attribute가 없을 때 CharacterMovement로 폴백할지
	UPROPERTY(EditDefaultsOnly, Category="PhaseShift|Config")
	bool bUseCharacterMovementFallback = true;

private:
	// ===== 런타임 핸들 =====
	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> WaitDelayTask = nullptr;

	// 로컬 FOV 원복용
	float CachedOriginalFOV = -1.0f;

	// 폴백 MaxWalkSpeed 원복용
	float CachedOriginalMaxWalkSpeed = -1.0f;

	// 적용된 GE 핸들(있으면 제거)
	FActiveGameplayEffectHandle AppliedGEHandle;
	
	

private:
	bool TryApplyMoveSpeedGE(const FGameplayAbilityActorInfo* ActorInfo);
	bool TryApplyCharacterMovementFallback(const FGameplayAbilityActorInfo* ActorInfo);
	void ApplyLocalFOV(const FGameplayAbilityActorInfo* ActorInfo);
	void RestoreLocalFOV(const FGameplayAbilityActorInfo* ActorInfo);
	void RestoreCharacterMovementFallback(const FGameplayAbilityActorInfo* ActorInfo);
};
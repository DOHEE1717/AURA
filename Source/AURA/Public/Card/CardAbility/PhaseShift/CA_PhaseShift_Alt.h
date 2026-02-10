#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "CA_PhaseShift_Alt.generated.h"

UCLASS()
class AURA_API UCA_PhaseShift_Alt : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UCA_PhaseShift_Alt();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// 저장 만료 시간(0이면 만료 없음). 일단 0으로 두고 나중에 3초 등으로 바꿔도 됨.
	UPROPERTY(EditDefaultsOnly, Category="PhaseShift|Alt")
	float RecallExpireSeconds = 0.f;

private:
	class UPhaseShiftRecallComponent* GetRecallComponentFromActorInfo(const FGameplayAbilityActorInfo* ActorInfo) const;
	AActor* GetPawnAvatarFromActorInfo(const FGameplayAbilityActorInfo* ActorInfo) const;
};
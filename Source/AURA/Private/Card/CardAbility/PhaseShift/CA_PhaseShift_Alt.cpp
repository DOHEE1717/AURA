#include "Card/CardAbility/PhaseShift/CA_PhaseShift_Alt.h"

#include "GameBase/AuraPlayerState.h"
#include "Card/CardAbility/PhaseShift/PhaseShiftRecallComponent.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

UCA_PhaseShift_Alt::UCA_PhaseShift_Alt()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UCA_PhaseShift_Alt::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!ActorInfo)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UPhaseShiftRecallComponent* RecallComp = GetRecallComponentFromActorInfo(ActorInfo);
	AActor* PawnActor = GetPawnAvatarFromActorInfo(ActorInfo);

	if (!RecallComp || !PawnActor)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1) 이미 저장이 있으면 -> Recall
	if (RecallComp->HasSavedLocation())
	{
		RecallComp->RecallToSavedLocation(PawnActor, true);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// 2) 저장이 없으면 -> Save
	if (RecallExpireSeconds > 0.f)
	{
		RecallComp->SetExpireSeconds(RecallExpireSeconds);
	}

	RecallComp->SaveCurrentLocation(PawnActor);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

UPhaseShiftRecallComponent* UCA_PhaseShift_Alt::GetRecallComponentFromActorInfo(const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!ActorInfo) return nullptr;

	AAuraPlayerState* PS = Cast<AAuraPlayerState>(ActorInfo->OwnerActor.Get());
	if (!PS) return nullptr;

	return PS->GetPhaseShiftRecallComponent();
}

AActor* UCA_PhaseShift_Alt::GetPawnAvatarFromActorInfo(const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!ActorInfo) return nullptr;

	// Avatar가 PlayerState로 잡히는 케이스가 있어 PC->Pawn으로 강제
	if (APlayerController* PC = Cast<APlayerController>(ActorInfo->PlayerController.Get()))
	{
		return PC->GetPawn();
	}

	// fallback
	return Cast<AActor>(ActorInfo->AvatarActor.Get());
}
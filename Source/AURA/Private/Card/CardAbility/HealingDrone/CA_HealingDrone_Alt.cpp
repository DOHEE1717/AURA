// Fill out your copyright notice in the Description page of Project Settings.

#include "Card/CardAbility/HealingDrone/CA_HealingDrone_Alt.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#include "Card/CardAbility/HealingDrone/HealingShieldActor.h"

UCA_HealingDrone_Alt::UCA_HealingDrone_Alt()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UCA_HealingDrone_Alt::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 커밋 실패면 즉시 종료
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

	if (!HealingShieldActorClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CA_HealingDrone_Alt] HealingShieldActorClass is null."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 중복 발동 방지: 남아있으면 정리 (참조만)
	CleanupSpawnedShield();

	// ===== 스폰/어태치 기준 Pawn 확정 =====
	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());

	// Avatar가 Pawn이 아닐 수도 있어서 PC의 Pawn으로 fallback
	if (!Pawn && ActorInfo->PlayerController.IsValid())
	{
		Pawn = ActorInfo->PlayerController->GetPawn();
	}

	if (!Pawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CA_HealingDrone_Alt] Pawn is null (cannot spawn shield)."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1) 보호막 스폰 (플레이어 위치)
	{
		FActorSpawnParameters Params;
		Params.Owner = Pawn;
		Params.Instigator = Pawn;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		const FVector SpawnLoc = Pawn->GetActorLocation();
		const FRotator SpawnRot = FRotator::ZeroRotator;

		SpawnedShieldActor = World->SpawnActor<AHealingShieldActor>(
			HealingShieldActorClass,
			SpawnLoc,
			SpawnRot,
			Params
		);

		if (!SpawnedShieldActor)
		{
			UE_LOG(LogTemp, Warning, TEXT("[CA_HealingDrone_Alt] Failed to spawn ShieldActor."));
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}

		// Ability는 즉시 끝내지만, ShieldActor가 파괴될 때 참조 정리용으로만 둠
		SpawnedShieldActor->OnDestroyed.AddDynamic(this, &ThisClass::OnShieldDestroyed);

		// 플레이어에 붙여서 “항상 주변에”
		if (bAttachShieldToOwner)
		{
			SpawnedShieldActor->AttachToActor(
				Pawn,
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				ShieldAttachSocketName
			);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[CA_HealingDrone_Alt] Spawned Shield. EndAbility immediately (repro/cooldown works)."));

	// 쿨다운/재생산을 위해 즉시 종료
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UCA_HealingDrone_Alt::OnShieldDestroyed(AActor* DestroyedActor)
{
	UE_LOG(LogTemp, Log, TEXT("[CA_HealingDrone_Alt] Shield destroyed."));
	SpawnedShieldActor = nullptr;
}

void UCA_HealingDrone_Alt::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	CleanupSpawnedShield();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UCA_HealingDrone_Alt::CleanupSpawnedShield()
{
	if (SpawnedShieldActor)
	{
		SpawnedShieldActor->OnDestroyed.RemoveDynamic(this, &ThisClass::OnShieldDestroyed);
		SpawnedShieldActor = nullptr;
	}
}
// Fill out your copyright notice in the Description page of Project Settings.


#include "Card/CardAbility/OrbitalStrike/CA_OrbitalStrike_Alt.h"
#include "Card/CardAbility/OrbitalStrike/OrbitalReconComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "GameFramework/Pawn.h"
#include "GameplayTagsManager.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"

UCA_OrbitalStrike_Alt::UCA_OrbitalStrike_Alt()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// 프로젝트에서 이미 쓰고 있는 라우팅 태그(기본값)
	TargetingActiveTag = FGameplayTag::RequestGameplayTag(TEXT("State.Targeting.Active"), false);
	CombatStateTag     = FGameplayTag::RequestGameplayTag(TEXT("State.Combat.Active"), false);
}

void UCA_OrbitalStrike_Alt::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)

{
	UE_LOG(LogTemp, Warning, TEXT("[ReconAlt] ActivateAbility ENTER"));
	
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 코스트/쿨다운 등
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	// ===== Force ReconView Open (combat/non-combat regardless) =====
	UOrbitalReconComponent* ReconComp = nullptr;

	// PlayerState(Owner) 우선
	if (ActorInfo->OwnerActor.IsValid())
	{
		ReconComp = ActorInfo->OwnerActor->FindComponentByClass<UOrbitalReconComponent>();
	}

	// Pawn/Avatar 보조
	if (!ReconComp && ActorInfo->AvatarActor.IsValid())
	{
		ReconComp = ActorInfo->AvatarActor->FindComponentByClass<UOrbitalReconComponent>();
	}

	UE_LOG(LogTemp, Warning, TEXT("[ReconAlt] ReconComp=%s Owner=%s Avatar=%s"),
		*GetNameSafe(ReconComp),
		*GetNameSafe(ActorInfo->OwnerActor.Get()),
		*GetNameSafe(ActorInfo->AvatarActor.Get()));

	if (ReconComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ReconAlt] OpenReconView CALL"));
		ReconComp->OpenReconView();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[ReconAlt] ReconComp NOT FOUND (PlayerState에 OrbitalReconComponent가 붙었는지 확인)"));
	}

	// Targeting 입력 라우팅 ON // 나중에 전투, 비전투 구분해서 스킬 구분할 때 사용 가능하게 개발은 해둠
	// if (TargetingActiveTag.IsValid())
	// {
	// 	ASC->AddLooseGameplayTag(TargetingActiveTag);
	// }

	// TargetActor 스폰/대기
	if (!TargetActorClass)
	{
		CleanupTargetingTag();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		CleanupTargetingTag();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = ActorInfo->AvatarActor.Get();
	Params.Instigator = ActorInfo->AvatarActor.IsValid() ? Cast<APawn>(ActorInfo->AvatarActor.Get()) : nullptr;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AGameplayAbilityTargetActor* TargetActor = World->SpawnActor<AGameplayAbilityTargetActor>(TargetActorClass, Params);
	if (!TargetActor)
	{
		CleanupTargetingTag();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	TargetActor->StartTargeting(this);

	UAbilityTask_WaitTargetData* WaitTask = UAbilityTask_WaitTargetData::WaitTargetDataUsingActor(
		this,
		NAME_None,
		EGameplayTargetingConfirmation::UserConfirmed,
		TargetActor);

	WaitTask->ValidData.AddDynamic(this, &ThisClass::OnTargetDataReady);
	WaitTask->Cancelled.AddDynamic(this, &ThisClass::OnTargetDataCancelled);
	WaitTask->ReadyForActivation();
}

void UCA_OrbitalStrike_Alt::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	CleanupTargetingTag();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UCA_OrbitalStrike_Alt::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& DataHandle)
{
	const FGameplayAbilityActorInfo* ActorInfo = CurrentActorInfo;
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

	FVector Center = ActorInfo->AvatarActor.IsValid()
	? ActorInfo->AvatarActor->GetActorLocation()
	: FVector::ZeroVector;

	FHitResult HR;
	if (DataHandle.Data.Num() > 0 && DataHandle.Data[0].IsValid())
	{
		if (const FGameplayAbilityTargetData_SingleTargetHit* HitData =
			static_cast<const FGameplayAbilityTargetData_SingleTargetHit*>(DataHandle.Data[0].Get()))
		{
			HR = HitData->HitResult;
			Center = HR.ImpactPoint;
		}
		else
		{
			// 혹시 Location 기반 타겟데이터가 올 경우 대비(센터만 확보)
			const FVector Loc = DataHandle.Data[0]->GetEndPoint();
			if (!Loc.IsNearlyZero())
			{
				Center = Loc;
			}
		}
	}

	// 전투 판정:
	// - CombatStateTag가 유효하면 ASC 태그로 판정
	// - 태그가 무효면 기본은 전투 모드로 간주(비전투 UI로 튀는 것 방지)
	bool bCombatMode = true;
	if (ASC && CombatStateTag.IsValid())
	{
		bCombatMode = ASC->HasMatchingGameplayTag(CombatStateTag);
	}

	// ReconComponent 찾기: PlayerState(Owner) 우선 → Avatar 보조
	UOrbitalReconComponent* ReconComp = nullptr;
	if (ActorInfo->OwnerActor.IsValid())
	{
		ReconComp = ActorInfo->OwnerActor->FindComponentByClass<UOrbitalReconComponent>();
	}
	if (!ReconComp && ActorInfo->AvatarActor.IsValid())
	{
		ReconComp = ActorInfo->AvatarActor->FindComponentByClass<UOrbitalReconComponent>();
	}

	if (ReconComp)
	{
		// (유지) 전투/비전투 판정은 나중 확장용으로 그대로 둠
		ReconComp->ExecuteReconScan(Center, ReconRadius, bCombatMode);

		// 전투/비전투 상관없이 항상 ReconView 진입
		UE_LOG(LogTemp, Warning, TEXT("[ReconAlt] OpenReconView CALL ReconComp=%s"),
	*GetNameSafe(ReconComp));
		ReconComp->OpenReconView();
	}

	CleanupTargetingTag();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UCA_OrbitalStrike_Alt::OnTargetDataCancelled(const FGameplayAbilityTargetDataHandle& DataHandle)
{
	CleanupTargetingTag();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UCA_OrbitalStrike_Alt::CleanupTargetingTag()
{
	if (CurrentActorInfo && CurrentActorInfo->AbilitySystemComponent.IsValid())
	{
		if (TargetingActiveTag.IsValid() &&
			CurrentActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(TargetingActiveTag))
		{
			CurrentActorInfo->AbilitySystemComponent->RemoveLooseGameplayTag(TargetingActiveTag);
		}
	}
}
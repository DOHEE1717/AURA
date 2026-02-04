// Fill out your copyright notice in the Description page of Project Settings.


#include "Card/CardAbility/OrbitalStrike/CA_OrbitalStrike_Primary.h"

#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "Abilities/GameplayAbilityTypes.h"

#include "GameplayTagContainer.h"
#include "AbilitySystemComponent.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

#include "GameBase/AuraGameplayTags.h"
#include "Card/CardAbility/OrbitalStrike/OrbitalStrikeActor.h"

UCA_OrbitalStrike_Primary::UCA_OrbitalStrike_Primary()
{
	// GravityField와 동일: 두 번째 클릭으로 확정
	ConfirmationType = EGameplayTargetingConfirmation::UserConfirmed;

	// 에디터에서 BP_TA_OrbitalStrike 지정 예정
	TargetActorClass = nullptr;

	// 에디터에서 BP_OrbitalStrikeActor 지정 예정
	OrbitalStrikeActorClass = nullptr;
}

void UCA_OrbitalStrike_Primary::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// (권장) 커밋 실패면 즉시 종료
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!TargetActorClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CA_OrbitalStrike_Primary] TargetActorClass is null."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 이미 남아있으면 정리(중복 발동 방지)
	if (SpawnedTargetActor)
	{
		SpawnedTargetActor->Destroy();
		SpawnedTargetActor = nullptr;
	}
	if (WaitTargetDataTask)
	{
		WaitTargetDataTask->EndTask();
		WaitTargetDataTask = nullptr;
	}

	// 1) TargetActor 스폰
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = ActorInfo->OwnerActor.Get();
	SpawnParams.Instigator = Cast<APawn>(ActorInfo->AvatarActor.Get());
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FTransform SpawnTM = ActorInfo->AvatarActor->GetActorTransform();

	SpawnedTargetActor =
		World->SpawnActor<AGameplayAbilityTargetActor>(TargetActorClass, SpawnTM, SpawnParams);

	if (!SpawnedTargetActor)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// TargetActor 기본 세팅 (프리뷰 갱신/입력 연결용)
	SpawnedTargetActor->SetActorHiddenInGame(false);
	SpawnedTargetActor->SetActorTickEnabled(true);
	SpawnedTargetActor->StartTargeting(this);

	// 2) WaitTargetDataUsingActor
	WaitTargetDataTask =
		UAbilityTask_WaitTargetData::WaitTargetDataUsingActor(
			this,
			NAME_None,
			ConfirmationType,
			SpawnedTargetActor
		);

	if (!WaitTargetDataTask)
	{
		SpawnedTargetActor->Destroy();
		SpawnedTargetActor = nullptr;

		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	WaitTargetDataTask->ValidData.AddDynamic(this, &ThisClass::OnTargetDataReady);
	WaitTargetDataTask->Cancelled.AddDynamic(this, &ThisClass::OnTargetDataCancelled);
	WaitTargetDataTask->ReadyForActivation();

	// Targeting 입력 라우팅용 태그 (GravityField와 동일)
	if (ActorInfo && ActorInfo->IsLocallyControlled())
	{
		if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
		{
			ASC->AddLooseGameplayTag(TAG_TargetingActive);

			UE_LOG(LogTemp, Warning,
				TEXT("[CA_OrbitalStrike_Primary] Add Tag | TagValid=%d HasAfterAdd=%d ASC=%s ASCPtr=%p Owner=%s"),
				TAG_TargetingActive.IsValid() ? 1 : 0,
				ASC->HasMatchingGameplayTag(TAG_TargetingActive) ? 1 : 0,
				*GetNameSafe(ASC),
				ASC,
				*GetNameSafe(ASC->GetOwner()));
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[CA_OrbitalStrike_Primary] Activated. Waiting for target confirm..."));
}

void UCA_OrbitalStrike_Primary::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 1) 태그 먼저 제거 (Super 전에!)
	if (ActorInfo && ActorInfo->IsLocallyControlled())
	{
		if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
		{
			ASC->RemoveLooseGameplayTag(TAG_TargetingActive);
			UE_LOG(LogTemp, Warning, TEXT("[CA_OrbitalStrike_Primary] Remove Tag: State.Targeting.Active"));
		}
	}

	// 2) Task/TA 정리
	if (WaitTargetDataTask)
	{
		WaitTargetDataTask->EndTask();
		WaitTargetDataTask = nullptr;
	}

	if (SpawnedTargetActor)
	{
		SpawnedTargetActor->Destroy();
		SpawnedTargetActor = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UCA_OrbitalStrike_Primary::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& DataHandle)
{
	UE_LOG(LogTemp, Log, TEXT("[CA_OrbitalStrike_Primary] Target confirmed (2nd click)."));
	LogTargetData(DataHandle);

	// HitLocation 뽑기 (GravityField와 동일)
	FVector SpawnLoc = FVector::ZeroVector;

	if (DataHandle.Num() > 0)
	{
		const FGameplayAbilityTargetData* Data = DataHandle.Get(0);
		if (Data)
		{
			if (const FGameplayAbilityTargetData_SingleTargetHit* HitData =
				static_cast<const FGameplayAbilityTargetData_SingleTargetHit*>(Data))
			{
				SpawnLoc = HitData->HitResult.ImpactPoint;
			}
		}
	}

	if (!OrbitalStrikeActorClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CA_OrbitalStrike_Primary] OrbitalStrikeActorClass is null."));
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = GetAvatarActorFromActorInfo();
	Params.Instigator = Cast<APawn>(GetAvatarActorFromActorInfo());
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	World->SpawnActor<AOrbitalStrikeActor>(OrbitalStrikeActorClass, SpawnLoc, FRotator::ZeroRotator, Params);


	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UCA_OrbitalStrike_Primary::OnTargetDataCancelled(const FGameplayAbilityTargetDataHandle& DataHandle)
{
	UE_LOG(LogTemp, Log, TEXT("[CA_OrbitalStrike_Primary] Target cancelled."));
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UCA_OrbitalStrike_Primary::LogTargetData(const FGameplayAbilityTargetDataHandle& DataHandle) const
{
	if (DataHandle.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CA_OrbitalStrike_Primary] TargetData empty."));
		return;
	}

	const FGameplayAbilityTargetData* Data = DataHandle.Get(0);
	if (!Data)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CA_OrbitalStrike_Primary] TargetData null."));
		return;
	}

	if (const FGameplayAbilityTargetData_SingleTargetHit* HitData =
		static_cast<const FGameplayAbilityTargetData_SingleTargetHit*>(Data))
	{
		const FHitResult& HR = HitData->HitResult;
		UE_LOG(LogTemp, Log, TEXT("[CA_OrbitalStrike_Primary] HitLocation=%s Actor=%s"),
			*HR.ImpactPoint.ToString(),
			HR.GetActor() ? *HR.GetActor()->GetName() : TEXT("None"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[CA_OrbitalStrike_Primary] TargetData type not SingleTargetHit (Num=%d)."), DataHandle.Num());
}
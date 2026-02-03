// Fill out your copyright notice in the Description page of Project Settings.


#include "Card/CardAbility/GravityField/CA_GravityField_Alt.h"

#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "AbilitySystemComponent.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

#include "GameBase/AuraGameplayTags.h" 

#include "Card/CardAbility/GravityField/GravityFieldReverseActor.h"

UCA_GravityField_Alt::UCA_GravityField_Alt()
{
	ConfirmationType = EGameplayTargetingConfirmation::UserConfirmed;

	// 에디터에서 BP_TA_GravityField를 지정 (Primary와 동일)
	TargetActorClass = nullptr;
}

void UCA_GravityField_Alt::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// Primary와 동일: Super 호출
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
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 이미 남아있으면 정리(중복 발동 방지) - Primary와 동일
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

	// 1) TargetActor 스폰 - Primary와 동일
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

	// TargetActor 기본 세팅 (프리뷰 갱신/입력 연결용) - Primary와 동일
	APlayerController* PC = ActorInfo->PlayerController.Get();

	SpawnedTargetActor->SetActorHiddenInGame(false);
	SpawnedTargetActor->SetActorTickEnabled(true);
	SpawnedTargetActor->StartTargeting(this);

	// 2) WaitTargetDataUsingActor - Primary와 동일
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

	// Local에서만 입력 라우팅 태그 추가 - Primary와 동일
	if (ActorInfo && ActorInfo->IsLocallyControlled())
	{
		if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
		{
			ASC->AddLooseGameplayTag(TAG_TargetingActive);

			UE_LOG(LogTemp, Warning,
				TEXT("[CA_GravityField_Alt] Add Tag | TagValid=%d HasAfterAdd=%d ASC=%s ASCPtr=%p Owner=%s"),
				TAG_TargetingActive.IsValid() ? 1 : 0,
				ASC->HasMatchingGameplayTag(TAG_TargetingActive) ? 1 : 0,
				*GetNameSafe(ASC),
				ASC,
				*GetNameSafe(ASC->GetOwner()));
		}
	}
}

void UCA_GravityField_Alt::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	// 1) 태그 먼저 제거 (Super 전에!) - Primary와 동일
	if (ActorInfo && ActorInfo->IsLocallyControlled())
	{
		if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
		{
			ASC->RemoveLooseGameplayTag(TAG_TargetingActive);
			UE_LOG(LogTemp, Warning, TEXT("[CA_GravityField_Alt] Remove Tag: State.Targeting.Active"));
		}
	}

	// 2) Task/TA 정리 - Primary와 동일
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

void UCA_GravityField_Alt::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& DataHandle)
{
	UE_LOG(LogTemp, Log, TEXT("[CA_GravityField_Alt] Target confirmed (2nd click)."));

	// HitLocation 뽑기 - Primary와 동일
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

	// 스폰: Alt는 ReverseActor 스폰
	if (GravityFieldReverseActorClass && GetWorld())
	{
		FActorSpawnParameters Params;
		Params.Owner = GetAvatarActorFromActorInfo();
		Params.Instigator = Cast<APawn>(GetAvatarActorFromActorInfo());
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		GetWorld()->SpawnActor<AGravityFieldReverseActor>(
			GravityFieldReverseActorClass,
			SpawnLoc,
			FRotator::ZeroRotator,
			Params
		);
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UCA_GravityField_Alt::OnTargetDataCancelled(const FGameplayAbilityTargetDataHandle& DataHandle)
{
	UE_LOG(LogTemp, Log, TEXT("[CA_GravityField_Alt] Target cancelled."));
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UCA_GravityField_Alt::LogTargetData(const FGameplayAbilityTargetDataHandle& DataHandle) const
{
}

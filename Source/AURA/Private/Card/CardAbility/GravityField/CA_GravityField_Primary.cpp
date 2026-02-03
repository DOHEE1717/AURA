// Fill out your copyright notice in the Description page of Project Settings.


#include "Card/CardAbility/GravityField/CA_GravityField_Primary.h"

#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "Abilities/GameplayAbilityTypes.h"

#include "GameplayTagContainer.h"
#include "AbilitySystemComponent.h"

#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

#include "Card/CardAbility/GravityField/GravityFieldActor.h"


static const FGameplayTag TAG_TargetingActive =
	FGameplayTag::RequestGameplayTag(FName("State.Targeting.Active"));




UCA_GravityField_Primary::UCA_GravityField_Primary()
{
	// 기본값: 두 번째 클릭으로 확정
	ConfirmationType = EGameplayTargetingConfirmation::UserConfirmed;
	
	// 에디터에서 BP_TA_GravityField를 지정
	TargetActorClass = nullptr;

	
}

void UCA_GravityField_Primary::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
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
	APlayerController* PC = ActorInfo->PlayerController.Get();
	
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
	
	if (ActorInfo && ActorInfo->IsLocallyControlled())
	{
		if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
		{
			ASC->AddLooseGameplayTag(TAG_TargetingActive);

			UE_LOG(LogTemp, Warning,
				TEXT("[CA_GravityField_Primary] Add Tag | TagValid=%d HasAfterAdd=%d ASC=%s ASCPtr=%p Owner=%s"),
				TAG_TargetingActive.IsValid() ? 1 : 0,
				ASC->HasMatchingGameplayTag(TAG_TargetingActive) ? 1 : 0,
				*GetNameSafe(ASC),
				ASC,
				*GetNameSafe(ASC->GetOwner()));
		}
	}
	
}

void UCA_GravityField_Primary::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	// 1) 태그 먼저 제거 (Super 전에!)
	if (ActorInfo && ActorInfo->IsLocallyControlled())
	{
		if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
		{
			ASC->RemoveLooseGameplayTag(TAG_TargetingActive);
			UE_LOG(LogTemp, Warning, TEXT("[CA_GravityField_Primary] Remove Tag: State.Targeting.Active"));
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

void UCA_GravityField_Primary::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& DataHandle)
{
	UE_LOG(LogTemp, Log, TEXT("[CA_GravityField_Primary] Target confirmed (2nd click)."));

	// HitLocation 뽑기
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

	// 스폰
	if (GravityFieldActorClass && GetWorld())
	{
		FActorSpawnParameters Params;
		Params.Owner = GetAvatarActorFromActorInfo();
		Params.Instigator = Cast<APawn>(GetAvatarActorFromActorInfo());
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		GetWorld()->SpawnActor<AGravityFieldActor>(GravityFieldActorClass, SpawnLoc, FRotator::ZeroRotator, Params);
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UCA_GravityField_Primary::OnTargetDataCancelled(const FGameplayAbilityTargetDataHandle& DataHandle)
{
	UE_LOG(LogTemp, Log, TEXT("[CA_GravityField_Primary] Target cancelled."));
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UCA_GravityField_Primary::LogTargetData(const FGameplayAbilityTargetDataHandle& DataHandle) const
{
	// 우리가 TargetActor에서 SingleTargetHit로 넣어줬으니, HitResult를 뽑아 로그로 확인 가능
	if (DataHandle.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CA_GravityField_Primary] TargetData empty."));
		return;
	}

	const FGameplayAbilityTargetData* Data = DataHandle.Get(0);
	if (!Data)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CA_GravityField_Primary] TargetData null."));
		return;
	}

	// SingleTargetHit 케이스 처리
	if (const FGameplayAbilityTargetData_SingleTargetHit* HitData = static_cast<const FGameplayAbilityTargetData_SingleTargetHit*>(Data))
	{
		const FHitResult& HR = HitData->HitResult;
		UE_LOG(LogTemp, Log, TEXT("[CA_GravityField_Primary] HitLocation = %s, Actor=%s"),
			*HR.ImpactPoint.ToString(),
			HR.GetActor() ? *HR.GetActor()->GetName() : TEXT("None"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[CA_GravityField_Primary] TargetData type not SingleTargetHit (Num=%d)."), DataHandle.Num());
}

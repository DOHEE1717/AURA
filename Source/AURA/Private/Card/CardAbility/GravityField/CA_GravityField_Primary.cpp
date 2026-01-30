// Fill out your copyright notice in the Description page of Project Settings.


#include "Card/CardAbility/GravityField/CA_GravityField_Primary.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameFramework/PlayerController.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "Engine/World.h"


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
	
	UE_LOG(LogTemp, Warning, TEXT("[CA_GravityField_Primary] ActivateAbility: TargetActorClass=%s"),
		*GetNameSafe(TargetActorClass.Get()));

	UE_LOG(LogTemp, Warning, TEXT("[CA_GravityField_Primary] ConfirmType=%d IsLocallyControlled=%d HasAuthority=%d"),
		(int32)ConfirmationType.GetValue(),
		(ActorInfo && ActorInfo->IsLocallyControlled()) ? 1 : 0,
		(ActorInfo && ActorInfo->IsNetAuthority()) ? 1 : 0);

	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[CA_GravityField_Primary] Invalid ActorInfo/Avatar. EndAbility"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!TargetActorClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CA_GravityField_Primary] TargetActorClass is null. EndAbility"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CA_GravityField_Primary] World is null. EndAbility"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// ------------------------------------------------------------
	// 1) TargetActor를 "직접" 스폰 (아웃라이너에 반드시 나타나야 함)
	// ------------------------------------------------------------
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = ActorInfo->OwnerActor.Get();
	SpawnParams.Instigator = Cast<APawn>(ActorInfo->AvatarActor.Get());
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FTransform SpawnTM = ActorInfo->AvatarActor->GetActorTransform();

	AGameplayAbilityTargetActor* SpawnedTA =
		World->SpawnActor<AGameplayAbilityTargetActor>(TargetActorClass, SpawnTM, SpawnParams);

	UE_LOG(LogTemp, Warning, TEXT("[CA_GravityField_Primary] Spawn TargetActor -> %s"),
		*GetNameSafe(SpawnedTA));

	if (!SpawnedTA)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CA_GravityField_Primary] Failed to spawn TargetActor. EndAbility"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// TargetActor 초기 구동(프리뷰 갱신이 안 도는 케이스 대비)
	SpawnedTA->SetActorHiddenInGame(false);
	SpawnedTA->SetActorTickEnabled(true);
	SpawnedTA->StartTargeting(this);

	// ------------------------------------------------------------
	// 2) WaitTargetDataUsingActor 로 "이 인스턴스"를 Task에 연결
	// ------------------------------------------------------------
	UAbilityTask_WaitTargetData* Task =
		UAbilityTask_WaitTargetData::WaitTargetDataUsingActor(
			this,
			NAME_None,
			ConfirmationType,
			SpawnedTA
		);

	if (!Task)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CA_GravityField_Primary] WaitTargetDataUsingActor task creation failed. EndAbility"));
		// SpawnedTA는 TargetActor 정책에 따라 직접 Destroy가 필요할 수 있음
		SpawnedTA->Destroy();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Task->ValidData.AddDynamic(this, &ThisClass::OnTargetDataReady);
	Task->Cancelled.AddDynamic(this, &ThisClass::OnTargetDataCancelled);

	Task->ReadyForActivation();

	UE_LOG(LogTemp, Log, TEXT("[CA_GravityField_Primary] Targeting started (1st click should show decal)."));
	
}

void UCA_GravityField_Primary::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& DataHandle)
{
	UE_LOG(LogTemp, Log, TEXT("[CA_GravityField_Primary] Target confirmed (2nd click)."));

	LogTargetData(DataHandle);

	// 지금 단계에서는 스폰/이펙트 없음. “확정”만 확인.
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

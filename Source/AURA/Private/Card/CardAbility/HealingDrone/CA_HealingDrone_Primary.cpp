// Fill out your copyright notice in the Description page of Project Settings.


#include "Card/CardAbility/HealingDrone/CA_HealingDrone_Primary.h"

#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"

#include "Card/CardAbility/HealingDrone/HealingDroneActor.h" 

UCA_HealingDrone_Primary::UCA_HealingDrone_Primary()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UCA_HealingDrone_Primary::ActivateAbility(
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

	if (!HealingDroneActorClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CA_HealingDrone_Primary] HealingDroneActorClass is null."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 중복 발동 방지: 남아있으면 정리
	CleanupSpawnedDrone();

	APawn* Pawn = nullptr;

	// 1) Avatar가 Pawn이면 그게 정답
	Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());

	// 2) 아니면 PlayerController의 Pawn으로 강제
	if (!Pawn && ActorInfo && ActorInfo->PlayerController.IsValid())
	{
		Pawn = ActorInfo->PlayerController->GetPawn();
	}

	if (!Pawn)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* Avatar = Pawn; // 이후 코드는 Avatar 대신 Pawn(=플레이어 캐릭터) 기준으로

	// 1) 드론 스폰 (플레이어 주변)
	{
		FActorSpawnParameters Params;
		Params.Owner = Pawn;
		Params.Instigator = Pawn;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		const FVector SpawnLoc = Pawn->GetActorLocation() + Pawn->GetActorRotation().RotateVector(DroneSpawnOffset);
		const FRotator SpawnRot = Pawn->GetActorRotation();

		SpawnedDroneActor = World->SpawnActor<AHealingDroneActor>(HealingDroneActorClass, SpawnLoc, SpawnRot, Params);

		if (!SpawnedDroneActor)
		{
			UE_LOG(LogTemp, Warning, TEXT("[CA_HealingDrone_Primary] Failed to spawn HealingDroneActor."));
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}
	}

	// 2) “힐 레이저” 디버그라인 갱신 타이머 시작
	if (TraceInterval > 0.f)
	{
		World->GetTimerManager().SetTimer(
			TimerHandle_TickTrace,
			this,
			&ThisClass::TickHealTrace,
			TraceInterval,
			true
		);
	}

	// 3) HealDuration 후 종료 타이머
	if (HealDuration > 0.f)
	{
		World->GetTimerManager().SetTimer(
			TimerHandle_EndByDuration,
			this,
			&ThisClass::OnHealDurationFinished,
			HealDuration,
			false
		);
	}

	UE_LOG(LogTemp, Log, TEXT("[CA_HealingDrone_Primary] Spawned Drone. HealDuration=%.2f"), HealDuration);
}

void UCA_HealingDrone_Primary::TickHealTrace()
{
	if (!CurrentActorInfo)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	if (!SpawnedDroneActor)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// ===== 타겟 Pawn 확정 (고정 Actor 방지) =====
	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());

	// Avatar가 Pawn이 아닐 수도 있어서 PC Pawn으로 fallback
	if (!Pawn && CurrentActorInfo->PlayerController.IsValid())
	{
		Pawn = CurrentActorInfo->PlayerController->GetPawn();
	}

	if (!Pawn)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	const FVector Start = SpawnedDroneActor->GetActorLocation();
	const FVector Target = Pawn->GetActorLocation() + FVector(0, 0, 60.f);

	// “레이저 힐 느낌”: LineTrace + DebugLine
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HealingDroneTrace), false, SpawnedDroneActor);
	Params.AddIgnoredActor(Pawn); // 플레이어는 무시(시야 가림 방지)

	const FVector Dir = (Target - Start).GetSafeNormal();
	const FVector End = Start + Dir * TraceDistance;

	const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

	const float Life = FMath::Max(TraceInterval * 1.2f, 0.02f);

	if (bHit)
	{
		DrawDebugLine(World, Start, Hit.ImpactPoint, FColor::Green, false, Life, 0, 2.f);
		DrawDebugPoint(World, Hit.ImpactPoint, 6.f, FColor::Green, false, Life);
	}
	else
	{
		DrawDebugLine(World, Start, End, FColor::Green, false, Life, 0, 2.f);
	}
}

void UCA_HealingDrone_Primary::OnHealDurationFinished()
{
	UE_LOG(LogTemp, Log, TEXT("[CA_HealingDrone_Primary] HealDuration finished -> EndAbility"));
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UCA_HealingDrone_Primary::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 타이머 정리
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimerHandle_TickTrace);
		World->GetTimerManager().ClearTimer(TimerHandle_EndByDuration);
	}

	// 드론 정리
	CleanupSpawnedDrone();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UCA_HealingDrone_Primary::CleanupSpawnedDrone()
{
	if (SpawnedDroneActor)
	{
		SpawnedDroneActor->Destroy();
		SpawnedDroneActor = nullptr;
	}
}
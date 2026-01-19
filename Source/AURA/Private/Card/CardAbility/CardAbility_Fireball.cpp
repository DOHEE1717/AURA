// Fill out your copyright notice in the Description page of Project Settings.


#include "Card/CardAbility/CardAbility_Fireball.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Card/Actor/FireballProjectile.h"

UCardAbility_Fireball::UCardAbility_Fireball()
{
	
	InstancingPolicy=EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UCardAbility_Fireball::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActiveInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActiveInfo, true, false);
		return;
	}

	// 1) 코스트/쿨다운(설정했다면) 커밋
	//    (지금은 단순 발사만 하므로 커밋 실패 시 종료만 처리)
	if (!CommitAbility(Handle, ActorInfo, ActiveInfo))
	{
		EndAbility(Handle, ActorInfo, ActiveInfo, true, false);
		return;
	}

	// 2) 스폰 클래스 체크
	if (!FireballClass)
	{
		EndAbility(Handle, ActorInfo, ActiveInfo, true, false);
		return;
	}

	AActor* Avatar = ActorInfo->AvatarActor.Get();
	UWorld* World = Avatar->GetWorld();
	if (!World)
	{
		EndAbility(Handle, ActorInfo, ActiveInfo, true, false);
		return;
	}

	// 3) 발사 위치/방향 계산(캐릭터 전방)
	const FVector Forward = Avatar->GetActorForwardVector();
	const FVector SpawnLoc = Avatar->GetActorLocation() + Forward * SpawnOffset.X + FVector(0.f, 0.f, SpawnOffset.Z);
	const FRotator SpawnRot = Forward.Rotation();

	FActorSpawnParameters Params;
	Params.Owner = Avatar;
	Params.Instigator = Cast<APawn>(Avatar);

	// 4) 파이어볼 스폰
	World->SpawnActor<AFireballProjectile>(FireballClass, SpawnLoc, SpawnRot, Params);

	// 5) 즉시 종료(발사는 끝)
	EndAbility(Handle, ActorInfo, ActiveInfo, false, false);
	
}


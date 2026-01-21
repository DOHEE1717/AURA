// Fill out your copyright notice in the Description page of Project Settings.


#include "Card/CardAbility/CardAbility_Fireball.h"

#include "EditorCategoryUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
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
	Super::ActivateAbility(Handle, ActorInfo, ActiveInfo, TriggerEventData);
	
	//유효성 체크
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
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
	
	// 카메라 기준방향 조준
	APlayerController* PC=ActorInfo->PlayerController.IsValid()
	? Cast<APlayerController>(ActorInfo->PlayerController.Get())
	: nullptr;
	
	//유효성 검사
	if (!PC)
	{
		if (APawn* Pawn=Cast<APawn>(Avatar))
		{
			PC=Cast<APlayerController>(Pawn->GetController());
		}
	}
	
	FVector ViewLoc=Avatar->GetActorLocation();
	FRotator ViewRot=Avatar->GetActorRotation();
	
	if (PC)
	{
		PC->GetPlayerViewPoint(ViewLoc,ViewRot);
	}

	const FVector Forward = ViewRot.Vector();
	const FVector Right = FRotationMatrix(ViewRot).GetScaledAxis(EAxis::X);
	const FVector Up = FVector::UpVector;
	
	const FVector SpawnLoc = ViewLoc+Forward*SpawnOffset.X+Right*SpawnOffset.Y+Up*SpawnOffset.Z;
	
	const FRotator SpawnRot = ViewRot;
	
	FActorSpawnParameters Params;
	Params.Owner = Avatar;
	Params.Instigator = Cast<APawn>(Avatar);
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	

	// 4) 파이어볼 스폰
	AFireballProjectile* Projectile = World->SpawnActor<AFireballProjectile>(
		FireballClass, SpawnLoc, SpawnRot, Params);

	// 6) 방향/속도 주입 (Projectile 쪽에 구현)
	if (Projectile)
	{
		Projectile->FireInDirection(Forward, FireballSpeed);
		
	}

	// 7) 즉시 종료
	EndAbility(Handle, ActorInfo, ActiveInfo, false, false);
	
}


// Fill out your copyright notice in the Description page of Project Settings.


#include "Card/CardAbility/OrbitalStrike/OrbitalStrikeActor.h"

#include "Components/SphereComponent.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

// Sets default values
AOrbitalStrikeActor::AOrbitalStrikeActor()
{
 	
	PrimaryActorTick.bCanEverTick = true;
	
	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	SetRootComponent(Sphere);

	
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sphere->SetCollisionObjectType(ECC_WorldDynamic);
	Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);

	Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Sphere->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
	Sphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);

	Sphere->InitSphereRadius(Radius);
	Sphere->SetGenerateOverlapEvents(true);

	// 조사 범위 데칼(지면 표시)
	AreaDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("AreaDecal"));
	AreaDecal->SetupAttachment(RootComponent);
	AreaDecal->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	AreaDecal->DecalSize = DecalSize;
	AreaDecal->SetFadeScreenSize(0.001f);

	InitialLifeSpan = 0.f; // BeginPlay에서 LifeTime 사용

}


void AOrbitalStrikeActor::BeginPlay()
{
	Super::BeginPlay();
	
	// 데칼 머티리얼
	if (AreaDecal && AreaDecalMaterial)
	{
		AreaDecal->SetDecalMaterial(AreaDecalMaterial);
	}
	
	// // 데칼 크기 동기화 (UE: X=두께, Y/Z=가로세로)
	// if (AreaDecal)
	// {
	// 	const float Diameter = Radius * 2.0f;
	// 	AreaDecal->DecalSize = FVector(DecalSize.X, Diameter, Diameter);
	// }

	// 바닥 Z-fighting 방지
	FVector L = GetActorLocation();
	L.Z += DecalZOffset;
	SetActorLocation(L);

	Sphere->SetSphereRadius(Radius);

	if (LifeTime > 0.f)
	{
		SetLifeSpan(LifeTime);
	}

	CachedInstigatorPawn = GetInstigator();
	
}


void AOrbitalStrikeActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (DamageInterval <= 0.f)
	{
		ApplyDamageOnce();
		return;
	}

	TimeAccum += DeltaTime;
	if (TimeAccum >= DamageInterval)
	{
		TimeAccum = 0.f;
		ApplyDamageOnce();
	}
}

void AOrbitalStrikeActor::ApplyDamageOnce()
{
	if (!GetWorld()) return;

	TArray<AActor*> OverlappedActors;
	Sphere->GetOverlappingActors(OverlappedActors);

	int32 OverlapCount = OverlappedActors.Num();
	int32 PassedFilterCount = 0;
	int32 DamagedCount = 0;

	for (AActor* A : OverlappedActors)
	{
		if (ShouldIgnoreActor(A)) continue;

		USceneComponent* Root = A->GetRootComponent();
		if (!Root || Root->Mobility != EComponentMobility::Movable)
		{
			continue;
		}

		if (!PassesGroupFilter(A))
		{
			continue;
		}

		PassedFilterCount++;

		UGameplayStatics::ApplyDamage(
			A,
			DamagePerTick,
			CachedInstigatorPawn ? CachedInstigatorPawn->GetController() : nullptr,
			this,
			DamageTypeClass
		);

		DamagedCount++;
	}

	UE_LOG(LogTemp, Log, TEXT("[OrbitalStrike] ApplyTick | Overlap=%d Passed=%d Damaged=%d Radius=%.0f Dmg=%.1f Interval=%.2f"),
		OverlapCount, PassedFilterCount, DamagedCount, Radius, DamagePerTick, DamageInterval);
}

bool AOrbitalStrikeActor::ShouldIgnoreActor(const AActor* Actor) const
{
	if (!Actor) return true;
	if (Actor == this) return true;

	if (!IgnoreTag.IsNone() && Actor->ActorHasTag(IgnoreTag))
		return true;

	// Instigator 제외
	if (bExcludeInstigatorPawn && CachedInstigatorPawn)
	{
		if (Actor == CachedInstigatorPawn) return true;
	}

	// PlayerControlled 제외(Instigator가 아닐 수 있으니 별도)
	if (bExcludePlayerControlledPawns)
	{
		if (const APawn* P = Cast<APawn>(Actor))
		{
			if (P->IsPlayerControlled()) return true;
		}
	}

	return false;
}

bool AOrbitalStrikeActor::PassesGroupFilter(const AActor* Actor) const
{
	// Enemy / Summoned / Prop 중 하나라도 매칭되면 통과(스위치로 제어)
	const bool bIsEnemy = !EnemyActorTag.IsNone() && Actor->ActorHasTag(EnemyActorTag);
	const bool bIsSummoned = !SummonedActorTag.IsNone() && Actor->ActorHasTag(SummonedActorTag);
	const bool bIsProp = !PropActorTag.IsNone() && Actor->ActorHasTag(PropActorTag);

	if (bIsEnemy)   return bAffectEnemies;
	if (bIsSummoned) return bAffectSummoned;
	if (bIsProp)     return bAffectProps;

	// 어떤 그룹에도 속하지 않으면 기본 제외(원하면 true로 바꿀 수 있음)
	return false;
}


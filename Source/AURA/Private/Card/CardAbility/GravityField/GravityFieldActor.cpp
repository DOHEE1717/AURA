// Fill out your copyright notice in the Description page of Project Settings.


#include "Card/CardAbility/GravityField/GravityFieldActor.h"

#include "Components/SphereComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"


AGravityFieldActor::AGravityFieldActor()
{
 	PrimaryActorTick.bCanEverTick = true;
	
	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	SetRootComponent(Sphere);

	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sphere->SetCollisionObjectType(ECC_WorldDynamic);
	Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);

	// Pawn/PhysicsBody/WorldDynamic 정도만 잡는 편이 안전
	Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Sphere->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
	Sphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);

	Sphere->SetGenerateOverlapEvents(true);
	
	// ★ 지속 데칼 생성
	FieldDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("FieldDecal"));
	FieldDecal->SetupAttachment(RootComponent);

	// 바닥을 향하게 눕힘(데칼은 보통 -90 Pitch)
	FieldDecal->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	FieldDecal->DecalSize = DecalSize;

	// 너무 멀리서 안 보이는 현상 방지(필요 시)
	FieldDecal->SetFadeScreenSize(0.001f);

	InitialLifeSpan = 0.f; // BeginPlay에서 LifeTime으로 SetLifeSpan 사용

}


void AGravityFieldActor::BeginPlay()
{
	Super::BeginPlay();

	// 데칼 머티리얼 적용
	if (FieldDecal && FieldDecalMaterial)
	{
		FieldDecal->SetDecalMaterial(FieldDecalMaterial);
	}

	// 바닥 끼임 방지로 살짝 띄움
	FVector L = GetActorLocation();
	L.Z += DecalZOffset;
	SetActorLocation(L);

	// 반경과 데칼 크기 연동하고 싶으면(선택):
	// DecalSize = FVector(200.f, Radius, Radius); 같은 식으로 BP에서 튜닝 권장

	Sphere->SetSphereRadius(Radius);

	if (LifeTime > 0.f)
	{
		SetLifeSpan(LifeTime); // ★ 이 동안 데칼도 계속 보임
	}

	CachedInstigatorPawn = GetInstigator();
	
}


void AGravityFieldActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (ApplyInterval <= 0.f)
	{
		ApplyPullOnce();
		return;
	}

	TimeAccum += DeltaTime;
	if (TimeAccum >= ApplyInterval)
	{
		TimeAccum = 0.f;
		ApplyPullOnce();
	}

}

void AGravityFieldActor::ApplyPullOnce()
{
	UWorld* World = GetWorld();
	if (!World) return;

	const FVector Center = GetActorLocation();

	TArray<AActor*> OverlappedActors;
	Sphere->GetOverlappingActors(OverlappedActors);

	for (AActor* A : OverlappedActors)
	{
		if (ShouldIgnoreActor(A)) continue;
		
		// ===== 0) 월드맵(고정 지형) 제외: Movable이 아닌 Root는 스킵 =====
		USceneComponent* Root = A->GetRootComponent();
		if (!Root || Root->Mobility != EComponentMobility::Movable)
		{
			continue;
		}

		// ===== 1) 그룹 필터(Actor Tags 기반) =====
		const bool bIsEnemy = !EnemyActorTag.IsNone() && A->ActorHasTag(EnemyActorTag);
		const bool bIsSummoned = !SummonedActorTag.IsNone() && A->ActorHasTag(SummonedActorTag);
		const bool bIsProp = !PropActorTag.IsNone() && A->ActorHasTag(PropActorTag);

		// 아무 그룹에도 속하지 않으면 기본 제외(

		// 1) 캐릭터/폰(적) 처리: LaunchCharacter로 “빨려드는 느낌” 추가
		//    (순수 물리(1번)만 원하면 이 블록을 통째로 지워도 됨)
		if (ACharacter* Ch = Cast<ACharacter>(A))
		{
			// 플레이어는 Instigator로 이미 제외됨
			const FVector Dir = (Center - Ch->GetActorLocation());
			const float Dist = FMath::Max(Dir.Size(), 1.f);
			const FVector N = Dir / Dist;

			// 거리에 따라 약간 감쇠
			const float Alpha = FMath::Clamp(1.f - (Dist / Radius), 0.f, 1.f);
			const FVector LaunchVel = N * (CharacterPullStrength * Alpha);

			Ch->LaunchCharacter(LaunchVel, true, true);
			continue;
		}

		// 2) 물리 오브젝트(사물) 처리: SimulatePhysics인 Primitive에 Force 적용
		TArray<UPrimitiveComponent*> PrimComps;
		A->GetComponents<UPrimitiveComponent>(PrimComps);

		for (UPrimitiveComponent* Prim : PrimComps)
		{
			if (!Prim) continue;
			if (!Prim->IsSimulatingPhysics()) continue;

			const FVector Dir = (Center - Prim->GetComponentLocation());
			const float Dist = FMath::Max(Dir.Size(), 1.f);
			const FVector N = Dir / Dist;

			// 거리 감쇠(가까울수록 강하게)
			const float Alpha = FMath::Clamp(1.f - (Dist / Radius), 0.f, 1.f);
			const FVector Force = N * (PhysicsForce * Alpha);

			Prim->AddForce(Force, NAME_None, true);
		}
	}
}

bool AGravityFieldActor::ShouldIgnoreActor(const AActor* Actor) const
{
	if (!Actor) return true;
	if (Actor == this) return true;

	if (!IgnoreTag.IsNone() && Actor->ActorHasTag(IgnoreTag))
		return true;

	// 플레이어 제외
	if (bExcludeInstigatorPawn && CachedInstigatorPawn)
	{
		if (Actor == CachedInstigatorPawn) return true;
	}

	return false;
}


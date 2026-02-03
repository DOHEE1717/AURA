// Fill out your copyright notice in the Description page of Project Settings.


#include "Card/CardAbility/GravityField/GravityFieldReverseActor.h"

#include "Components/SphereComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/CharacterMovementComponent.h"

AGravityFieldReverseActor::AGravityFieldReverseActor()
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

	Sphere->SetGenerateOverlapEvents(true);

	FieldDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("FieldDecal"));
	FieldDecal->SetupAttachment(RootComponent);

	FieldDecal->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	FieldDecal->DecalSize = DecalSize;
	FieldDecal->SetFadeScreenSize(0.001f);

	InitialLifeSpan = 0.f;
}

void AGravityFieldReverseActor::BeginPlay()
{
	Super::BeginPlay();

	if (FieldDecal && FieldDecalMaterial)
	{
		FieldDecal->SetDecalMaterial(FieldDecalMaterial);
	}

	// 바닥 끼임 방지로 살짝 띄움
	FVector L = GetActorLocation();
	L.Z += DecalZOffset;
	SetActorLocation(L);

	Sphere->SetSphereRadius(Radius);

	if (LifeTime > 0.f)
	{
		SetLifeSpan(LifeTime);
	}

	CachedInstigatorPawn = GetInstigator();
	
	CachedHoverZ = GetActorLocation().Z + HoverZOffsetFromField;
}

void AGravityFieldReverseActor::Tick(float DeltaTime)
{
	LastTickDelta = DeltaTime;
	Super::Tick(DeltaTime);

	if (ApplyInterval <= 0.f)
	{
		ApplyLiftOnce();
		return;
	}

	TimeAccum += DeltaTime;
	if (TimeAccum >= ApplyInterval)
	{
		TimeAccum = 0.f;
		ApplyLiftOnce();
	}
}

void AGravityFieldReverseActor::ApplyLiftOnce()
{
	if (!Sphere) return;

	TArray<AActor*> OverlappedActors;
	Sphere->GetOverlappingActors(OverlappedActors);

	for (AActor* A : OverlappedActors)
	{
		if (ShouldIgnoreActor(A)) continue;

		// 캐릭터는 무조건 띄움 (내 캐릭터/적 캐릭터 모두)
		if (ACharacter* Ch = Cast<ACharacter>(A))
		{
			UE_LOG(LogTemp, Warning, TEXT("[ReverseField] Hit Character: %s PC=%d"),
			       *GetNameSafe(Ch),
			       (Ch->GetController() && Ch->GetController()->IsPlayerController()) ? 1 : 0);

			const float Dist = FVector::Dist(Ch->GetActorLocation(), GetActorLocation());
			const float Alpha = FMath::Clamp(1.f - (Dist / Radius), 0.f, 1.f);

			const float UpSpeed = CharacterLiftStrength * Alpha;

			// 접지/브레이킹으로 Launch가 씹히는 케이스 방지: 공중 모드로 전환
			if (UCharacterMovementComponent* Move = Ch->GetCharacterMovement())
			{
				Move->SetMovementMode(MOVE_Falling);
			}

			// XY는 유지하고 Z만 강제로 갱신(플레이어 입력/이동과 충돌 최소화)
			Ch->LaunchCharacter(FVector(0.f, 0.f, UpSpeed), false, true);
			
			
			
			continue;
		}
		

		// 물리 오브젝트(Prop 등): SimulatePhysics 켜진 컴포넌트에만 적용
		TArray<UPrimitiveComponent*> PrimComps;
		A->GetComponents<UPrimitiveComponent>(PrimComps);

		for (UPrimitiveComponent* Prim : PrimComps)
		{
			if (!Prim) continue;
			if (!Prim->IsSimulatingPhysics()) continue;

			const float Dist = FVector::Dist(Prim->GetComponentLocation(), GetActorLocation());
			const float Alpha = FMath::Clamp(1.f - (Dist / Radius), 0.f, 1.f);

			const FVector Force = FVector::UpVector * (PhysicsUpForce * Alpha);

			// bAccelChange=true : 질량 무시(가속도 기반) → 튜닝 쉬움
			Prim->AddForce(Force, NAME_None, true);
		}
	}
}

bool AGravityFieldReverseActor::ShouldIgnoreActor(const AActor* Actor) const
{
	if (!Actor) return true;
	if (Actor == this) return true;

	// 예외 태그만 스킵(원하면 IgnoreTag를 None으로)
	if (!IgnoreTag.IsNone() && Actor->ActorHasTag(IgnoreTag))
		return true;

	// 아래 2개는 “전부 띄우기”면 false로 두는 게 기본
	if (bExcludeInstigatorPawn && CachedInstigatorPawn && Actor == CachedInstigatorPawn)
		return true;

	if (bExcludePlayerControlledPawns)
	{
		if (const APawn* Pawn = Cast<APawn>(Actor))
		{
			if (Pawn->IsPlayerControlled())
				return true;
		}
	}

	return false;
}


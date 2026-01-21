// Fill out your copyright notice in the Description page of Project Settings.

#include "Card/Actor/FireballProjectile.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

AFireballProjectile::AFireballProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);

	Collision->InitSphereRadius(12.5f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = Collision;
	ProjectileMovement->InitialSpeed = 2000.f;
	ProjectileMovement->MaxSpeed = 2000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.f;
	ProjectileMovement->bShouldBounce = false;

	InitialLifeSpan = 3.0f;
}

void AFireballProjectile::BeginPlay()
{
	Super::BeginPlay();
}

void AFireballProjectile::FireInDirection(const FVector& Direction, float Speed)
{
	if (!ProjectileMovement || !Collision) return;

	const FVector Dir = Direction.GetSafeNormal();
	UE_LOG(LogTemp, Warning, TEXT("[FireballProj] FireInDirection Dir=%s Speed=%.1f BeforeVel=%s Active=%d Updated=%s"),
		*Dir.ToString(),
		Speed,
		*ProjectileMovement->Velocity.ToString(),
		ProjectileMovement->IsActive() ? 1 : 0,
		*GetNameSafe(ProjectileMovement->UpdatedComponent));

	ProjectileMovement->InitialSpeed = Speed;
	ProjectileMovement->MaxSpeed = Speed;

	// (중요) 로컬/월드 혼동 방지: UpdatedComponent 기준으로 속도 반영
	ProjectileMovement->Velocity = Dir * Speed;

	// (중요) 강제 활성화 + 속도 갱신
	ProjectileMovement->Activate(true);
	ProjectileMovement->UpdateComponentVelocity();

	UE_LOG(LogTemp, Warning, TEXT("[FireballProj] AfterVel=%s Active=%d"),
		*ProjectileMovement->Velocity.ToString(),
		ProjectileMovement->IsActive() ? 1 : 0);
}

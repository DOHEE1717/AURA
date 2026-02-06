// Fill out your copyright notice in the Description page of Project Settings.

#include "Card/CardAbility/HealingDrone/HealingShieldActor.h"

#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"

AHealingShieldActor::AHealingShieldActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	ShieldSphere = CreateDefaultSubobject<USphereComponent>(TEXT("ShieldSphere"));
	ShieldSphere->SetupAttachment(Root);

	// 최소 구현: 보호막은 “Overlap 영역”만 제공
	ShieldSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ShieldSphere->SetCollisionObjectType(ECC_WorldDynamic);
	ShieldSphere->SetCollisionResponseToAllChannels(ECR_Overlap);

	// 필요하면 Pawn만 Overlap로 좁힐 수도 있음(지금은 범용)
	// ShieldSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	// ShieldSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// 시각적 메시 없음(나중 폴리싱)
}

void AHealingShieldActor::BeginPlay()
{
	Super::BeginPlay();

	CurrentHP = MaxHP;
	SyncSphereRadius();

	if (bDrawDebug)
	{
		DrawDebugSphere(
			GetWorld(),
			GetActorLocation(),
			Radius,
			24,
			FColor::Cyan,
			false,
			1.0f,
			0,
			2.0f
		);
	}
}

void AHealingShieldActor::SyncSphereRadius()
{
	if (ShieldSphere)
	{
		ShieldSphere->SetSphereRadius(Radius, true);
	}
}

void AHealingShieldActor::ApplyDamage(float DamageAmount)
{
	if (DamageAmount <= 0.f) return;
	if (CurrentHP <= 0.f) return;

	CurrentHP -= DamageAmount;

	UE_LOG(LogTemp, Log, TEXT("[HealingShieldActor] Damage=%.2f -> HP=%.2f/%.2f"),
		DamageAmount, CurrentHP, MaxHP);

	if (bDrawDebug && GetWorld())
	{
		// 데미지 때마다 잠깐 갱신(짧게)
		DrawDebugSphere(
			GetWorld(),
			GetActorLocation(),
			Radius,
			24,
			FColor::Red,
			false,
			0.15f,
			0,
			2.0f
		);
	}

	if (CurrentHP <= 0.f)
	{
		Die();
	}
}

void AHealingShieldActor::BreakShield()
{
	if (CurrentHP <= 0.f) return;
	CurrentHP = 0.f;
	Die();
}

float AHealingShieldActor::GetHPRatio() const
{
	if (MaxHP <= 0.f) return 0.f;
	return FMath::Clamp(CurrentHP / MaxHP, 0.f, 1.f);
}

void AHealingShieldActor::Die()
{
	UE_LOG(LogTemp, Warning, TEXT("[HealingShieldActor] Shield broken -> Destroy"));

	// 여기서 이펙트/사운드 나중에 추가
	Destroy();
}

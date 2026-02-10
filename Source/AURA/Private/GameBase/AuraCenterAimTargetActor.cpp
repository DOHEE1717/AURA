// Fill out your copyright notice in the Description page of Project Settings.

#include "GameBase/AuraCenterAimTargetActor.h"

#include "Components/DecalComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

AAuraCenterAimTargetActor::AAuraCenterAimTargetActor()
{
	PrimaryActorTick.bCanEverTick = true;

	// Root
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Optional Decal (BP에서 교체 가능)
	TargetDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("TargetDecal"));
	TargetDecal->SetupAttachment(RootComponent);
	TargetDecal->DecalSize = FVector(128.f, 256.f, 256.f);
}

void AAuraCenterAimTargetActor::BeginPlay()
{
	Super::BeginPlay();
}

void AAuraCenterAimTargetActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 센터 에임은 매 Tick 갱신
	UpdateLocationFromViewCenter();
}

void AAuraCenterAimTargetActor::BuildIgnoreActors(TArray<AActor*>& OutIgnore) const
{
	// 자기 자신
	OutIgnore.Add(const_cast<AAuraCenterAimTargetActor*>(this));

	// Instigator / Owner 무시
	if (AActor* Inst = GetInstigator())
	{
		OutIgnore.Add(Inst);
	}
	if (AActor* OwnerActor = GetOwner())
	{
		OutIgnore.Add(OwnerActor);
	}

	// 실제 조종 Pawn도 무시 (Owner/Instigator 누락 대비)
	if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		OutIgnore.Add(Pawn);
	}
}

void AAuraCenterAimTargetActor::UpdateLocationFromViewCenter()
{
	UWorld* World = GetWorld();
	if (!World) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	// 카메라 기준 위치 / 방향
	FVector ViewLoc;
	FRotator ViewRot;
	PC->GetPlayerViewPoint(ViewLoc, ViewRot);

	const FVector Start = ViewLoc;
	const FVector End   = Start + (ViewRot.Vector() * MaxTraceDistance);

	TArray<AActor*> IgnoreActors;
	BuildIgnoreActors(IgnoreActors);

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CenterAimTrace), false);
	Params.AddIgnoredActors(IgnoreActors);
	Params.bTraceComplex = false;

	// A/A 확정:
	// - 채널: Visibility
	// - 실패 시: EndPoint 유지
	const bool bHit = World->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ECC_Visibility,
		Params
	);

	const FVector NewLoc = bHit ? Hit.ImpactPoint : End;

	// 최종 위치 적용
	SetActorLocation(NewLoc);

	// 데칼이 있으면 화면 기준으로 회전만 맞춰줌
	if (TargetDecal)
	{
		TargetDecal->SetWorldRotation(FRotator(-90.f, ViewRot.Yaw, 0.f));
	}

#if ENABLE_DRAW_DEBUG
	// 디버그: 센터 에임 라인
	DrawDebugLine(
		World,
		Start,
		bHit ? Hit.ImpactPoint : End,
		bHit ? FColor::Green : FColor::Red,
		false,
		0.f,
		0,
		1.f
	);
#endif
}
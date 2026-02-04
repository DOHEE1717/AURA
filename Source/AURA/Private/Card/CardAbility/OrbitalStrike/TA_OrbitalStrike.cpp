// Fill out your copyright notice in the Description page of Project Settings.


#include "Card/CardAbility/OrbitalStrike/TA_OrbitalStrike.h"

#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Components/DecalComponent.h"
#include "GameFramework/PlayerController.h"

ATA_OrbitalStrike::ATA_OrbitalStrike()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;

	PreviewDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("PreviewDecal"));
	SetRootComponent(PreviewDecal);

	PreviewDecal->SetHiddenInGame(true);
	PreviewDecal->DecalSize = FVector(Radius, Radius, Radius);
}

void ATA_OrbitalStrike::StartTargeting(UGameplayAbility* Ability)
{
	Super::StartTargeting(Ability);

	// PlayerController 캐시
	CachedPC = Ability ? Ability->GetCurrentActorInfo()->PlayerController.Get() : nullptr;

	// 머티리얼/사이즈 세팅
	if (PreviewDecal && PreviewDecalMaterial)
	{
		PreviewDecal->SetDecalMaterial(PreviewDecalMaterial);
	}

	if (PreviewDecal)
	{
		PreviewDecal->DecalSize = FVector(Radius, Radius, Radius);
		PreviewDecal->SetHiddenInGame(false);
	}

	UpdateAimHitResult();
	UpdatePreviewDecal();
}

void ATA_OrbitalStrike::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateAimHitResult();
	UpdatePreviewDecal();
}

void ATA_OrbitalStrike::ConfirmTargetingAndContinue()
{
	// TargetData 생성: SingleTargetHit로 넘김
	FGameplayAbilityTargetDataHandle Handle;
	Handle.Add(new FGameplayAbilityTargetData_SingleTargetHit(CachedHitResult));

	TargetDataReadyDelegate.Broadcast(Handle);

	if (PreviewDecal)
	{
		PreviewDecal->SetHiddenInGame(true);
	}

	// Confirm 후 자기 자신 정리
	Destroy();
}

void ATA_OrbitalStrike::CancelTargeting()
{
	if (PreviewDecal)
	{
		PreviewDecal->SetHiddenInGame(true);
	}

	CanceledDelegate.Broadcast(FGameplayAbilityTargetDataHandle());
	Destroy();
}

void ATA_OrbitalStrike::UpdateAimHitResult()
{
	APlayerController* PC = CachedPC.Get();
	if (!PC) return;

	FVector WorldLoc, WorldDir;
	if (!PC->DeprojectMousePositionToWorld(WorldLoc, WorldDir))
	{
		// fallback: 카메라 기준
		FVector CamLoc;
		FRotator CamRot;
		PC->GetPlayerViewPoint(CamLoc, CamRot);
		WorldLoc = CamLoc;
		WorldDir = CamRot.Vector();
	}

	const FVector Start = WorldLoc;
	const FVector End = Start + (WorldDir * TraceDistance);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(OrbitalStrikeTargetTrace), false);
	Params.AddIgnoredActor(PC->GetPawn());

	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit, Start, End, ECC_Visibility, Params
	);

	if (bHit)
	{
		CachedHitResult = Hit;
	}
	else
	{
		// 히트가 없으면 End를 임시 위치로
		CachedHitResult = FHitResult();
		CachedHitResult.Location = End;
		CachedHitResult.ImpactPoint = End;
	}
}

void ATA_OrbitalStrike::UpdatePreviewDecal()
{
	if (!PreviewDecal) return;

	FVector Loc = CachedHitResult.bBlockingHit ? CachedHitResult.ImpactPoint : CachedHitResult.Location;
	Loc.Z += 5.f;

	// 바닥용 데칼 회전
	const FRotator Rot(-90.f, 0.f, 0.f);

	PreviewDecal->SetWorldLocation(Loc);
	PreviewDecal->SetWorldRotation(Rot);

	// 반경 변경을 BP에서 런타임으로 바꿀 수도 있으니 매 프레임 갱신해도 OK
	PreviewDecal->DecalSize = FVector(Radius, Radius, Radius);
}
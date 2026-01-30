// Fill out your copyright notice in the Description page of Project Settings.


#include "Card/CardAbility/GravityField/TA_GravityField.h"

#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Components/DecalComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

ATA_GravityField::ATA_GravityField()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	
	PreviewDecal=CreateDefaultSubobject<UDecalComponent>(TEXT("PreviewDecal"));
	SetRootComponent(PreviewDecal);
	
	PreviewDecal->SetHiddenInGame(true);
	PreviewDecal->DecalSize=FVector(Radius,Radius,Radius);
	
}

void ATA_GravityField::StartTargeting(UGameplayAbility* Ability)
{
	Super::StartTargeting(Ability);
	
	//Playercontroller 찾기
	CachedPC=Ability?Ability->GetCurrentActorInfo()->PlayerController.Get():nullptr;
	
	//머터리얼, 사이즈 체크
	if (PreviewDecal&&PreviewDecalMaterial)
	{
		PreviewDecal->SetDecalMaterial(PreviewDecalMaterial);
	}
	
	if (PreviewDecal)
	{
		PreviewDecal->DecalSize=FVector(Radius,Radius,Radius);
		PreviewDecal->SetHiddenInGame(false);		
	}
	
	UpdateAimHitResult();
	UpdatePreviewDecal();
}

void ATA_GravityField::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	UpdateAimHitResult();
	UpdatePreviewDecal();
}

void ATA_GravityField::ConfirmTargetingAndContinue()
{
	// TargetData 생성: SingleTargetHit로 넘기면 Ability 쪽에서 HitResult를 그대로 사용 가능
	FGameplayAbilityTargetDataHandle Handle;
	Handle.Add(new FGameplayAbilityTargetData_SingleTargetHit(CachedHitResult));

	TargetDataReadyDelegate.Broadcast(Handle);

	if (PreviewDecal)
	{
		PreviewDecal->SetHiddenInGame(true);
	}

	// TargetActor는 보통 confirm 후 자기 자신을 정리
	Destroy();
}

void ATA_GravityField::CancelTargeting()
{
	if (PreviewDecal)
	{
		PreviewDecal->SetHiddenInGame(true);
	}

	CanceledDelegate.Broadcast(FGameplayAbilityTargetDataHandle());
	Destroy();
}

void ATA_GravityField::UpdateAimHitResult()
{
	APlayerController* PC = CachedPC.Get();
	if (!PC) return;

	FVector WorldLoc, WorldDir;
	if (!PC->DeprojectMousePositionToWorld(WorldLoc, WorldDir))
	{
		// 마우스 디프로젝션이 불가하면, 카메라 기준으로 라인트레이스(대안)
		FVector CamLoc;
		FRotator CamRot;
		PC->GetPlayerViewPoint(CamLoc, CamRot);
		WorldLoc = CamLoc;
		WorldDir = CamRot.Vector();
	}

	const FVector Start = WorldLoc;
	const FVector End = Start + (WorldDir * TraceDistance);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(GravityFieldTargetTrace), false);
	Params.AddIgnoredActor(PC->GetPawn());

	FHitResult Hit;
	bool bHit = GetWorld()->LineTraceSingleByChannel(
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

void ATA_GravityField::UpdatePreviewDecal()
{
	if (!PreviewDecal) return;

	// 바닥에 붙이는 느낌으로 약간 띄우기
	FVector Loc = CachedHitResult.bBlockingHit ? CachedHitResult.ImpactPoint : CachedHitResult.Location;
	Loc.Z += 5.f;

	// 디칼은 보통 바닥을 향하게 -Z 방향으로 회전
	const FRotator Rot = FRotator(-90.f, 0.f, 0.f);

	PreviewDecal->SetWorldLocation(Loc);
	PreviewDecal->SetWorldRotation(Rot);
}

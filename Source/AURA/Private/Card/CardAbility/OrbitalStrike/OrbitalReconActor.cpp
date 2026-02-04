// Fill out your copyright notice in the Description page of Project Settings.


#include "Card/CardAbility/OrbitalStrike/OrbitalReconActor.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

// Sets default values
AOrbitalReconActor::AOrbitalReconActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->bDoCollisionTest = false;
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->SetRelativeRotation(DefaultArmRotation);
	SpringArm->TargetArmLength = DefaultArmLength;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;
}

void AOrbitalReconActor::BeginPlay()
{
	Super::BeginPlay();
}

void AOrbitalReconActor::InitView(const FVector& InCenter, float InRadius)
{
	ScanCenter = InCenter;
	ScanRadius = InRadius;

	FVector CamTarget = ScanCenter;
	CamTarget.Z += CenterZOffset;

	SetActorLocation(CamTarget);

	// 반경이 커지면 조금 더 멀리서 보는 게 좋으니 단순 비례 보정(원하면 나중에 조정)
	SpringArm->TargetArmLength = FMath::Max(DefaultArmLength, ScanRadius * 3.0f);
	SpringArm->SetRelativeRotation(DefaultArmRotation);
}





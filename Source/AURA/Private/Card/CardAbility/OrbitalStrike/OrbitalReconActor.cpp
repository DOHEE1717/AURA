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

void AOrbitalReconActor::ApplyMoveInput(const FVector2D& MoveAxis, float DeltaSeconds)
{
	if (MoveAxis.IsNearlyZero()) return;

	// 캐릭터 기준 Yaw로 이동 방향 계산
	const FRotator RefRot(0.f, MoveReferenceYaw, 0.f);
	const FVector Forward = RefRot.Vector();                          // 캐릭터 정면
	const FVector Right   = FRotationMatrix(RefRot).GetUnitAxis(EAxis::Y); // 캐릭터 우측

	// MoveAxis: X = A/D, Y = W/S
	FVector Delta = (Right * MoveAxis.X) + (Forward * MoveAxis.Y);
	Delta.Z = 0.f;

	Delta = Delta.GetClampedToMaxSize(1.f) * MoveSpeed * DeltaSeconds;

	FVector NewLoc = GetActorLocation() + Delta;

	if (bClampToScanRadius && ScanRadius > 0.f)
	{
		const float Limit = FMath::Max(0.f, ScanRadius + ClampMargin);
		FVector Center = ScanCenter;
		Center.Z = NewLoc.Z;

		FVector To = NewLoc - Center;
		To.Z = 0.f;

		if (To.SizeSquared() > Limit * Limit)
		{
			To = To.GetSafeNormal() * Limit;
			NewLoc.X = Center.X + To.X;
			NewLoc.Y = Center.Y + To.Y;
		}
	}

	SetActorLocation(NewLoc);
}

void AOrbitalReconActor::ApplyZoomInput(float ZoomAxis, float DeltaSeconds)
{
	if (FMath::IsNearlyZero(ZoomAxis)) return;
	if (!SpringArm) return;

	const float NewLen = SpringArm->TargetArmLength + (-ZoomAxis * ZoomSpeed * DeltaSeconds);
	SpringArm->TargetArmLength = FMath::Clamp(NewLen, MinArmLength, MaxArmLength);
}

void AOrbitalReconActor::SetMoveReferenceYaw(float InYaw)
{ MoveReferenceYaw = InYaw; }

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





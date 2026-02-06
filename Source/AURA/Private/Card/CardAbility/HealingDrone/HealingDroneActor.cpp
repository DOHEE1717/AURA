#include "Card/CardAbility/HealingDrone/HealingDroneActor.h"

#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

AHealingDroneActor::AHealingDroneActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;   // 추가
	SetActorTickEnabled(true);  

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(Root);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AHealingDroneActor::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(true);
	ResolveFollowTarget();

	// 시작 시 드론이 “뚝” 움직이지 않게, 현재 위치를 기준으로 속도 초기화
	Velocity = FVector::ZeroVector;
	HoverTime = 0.f;
}

void AHealingDroneActor::Tick(float DeltaSeconds)
{
	UE_LOG(LogTemp, Warning, TEXT("[HealingDrone] Target=%s TargetLoc=%s"),
	*GetNameSafe(FollowTarget.Get()),
	FollowTarget.IsValid() ? *FollowTarget->GetActorLocation().ToString() : TEXT("None"));
	
	Super::Tick(DeltaSeconds);

	ResolveFollowTarget();
	if (!FollowTarget.IsValid()) return;

	UpdateFollow(DeltaSeconds);
	
	
}

void AHealingDroneActor::ResolveFollowTarget()
{
	// 이미 Pawn을 잡고 있으면 유지
	if (FollowTarget.IsValid())
	{
		// 혹시 Owner(안 움직임) 잡혀있던 경우를 대비해서,
		// Pawn이 아니면 다시 재탐색하도록 풀어줌
		if (!Cast<APawn>(FollowTarget.Get()))
		{
			FollowTarget = nullptr;
		}
		else
		{
			return;
		}
	}

	// 1) Instigator가 제일 확실 (움직이는 Pawn)
	if (APawn* Inst = GetInstigator())
	{
		FollowTarget = Inst;
		return;
	}

	// 2) Owner가 Pawn이면 사용
	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		FollowTarget = OwnerPawn;
		return;
	}

	// 3) 마지막 fallback
	if (APawn* P = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		FollowTarget = P;
	}
}

FVector AHealingDroneActor::ComputeDesiredLocation(float DeltaSeconds)
{
	AActor* Target = FollowTarget.Get();
	if (!Target) return GetActorLocation();

	// 타겟 회전(Yaw) 기준으로 LocalOffset을 월드로 변환
	const FRotator TargetRot = Target->GetActorRotation();
	const FVector WorldOffset = TargetRot.RotateVector(LocalOffset);

	// Hover
	HoverTime += DeltaSeconds;
	const float HoverZ = FMath::Sin(HoverTime * HoverFrequency * 2.f * PI) * HoverAmplitude;

	FVector Desired = Target->GetActorLocation() + WorldOffset;
	Desired.Z += HoverZ;

	return Desired;
}

void AHealingDroneActor::UpdateFollow(float DeltaSeconds)
{
	AActor* Target = FollowTarget.Get();
	if (!Target) return;

	const FVector Current = GetActorLocation();
	const FVector Desired = ComputeDesiredLocation(DeltaSeconds);

	// 거리 기반 “추적 시작/정지” 판정
	FVector Cur2D = Current;  Cur2D.Z = 0.f;
	FVector Des2D = Desired;  Des2D.Z = 0.f;
	const float Dist = FVector::Dist(Cur2D, Des2D);

	if (!bIsFollowing)
	{
		// 멀어지면 따라가기 시작
		if (Dist >= FollowStartDistance)
		{
			bIsFollowing = true;
		}
		else
		{
			// 유지 구간: 속도 자연 감쇠로 정지(미세 떨림 방지)
			Velocity = FMath::VInterpTo(Velocity, FVector::ZeroVector, DeltaSeconds, 10.f);
			return;
		}
	}
	else
	{
		// 충분히 가까워지면 추적 중지(유지로 전환)
		if (Dist <= FollowStopDistance)
		{
			bIsFollowing = false;

			// 멈추는 순간 너무 튀지 않게 살짝 감쇠
			Velocity = FMath::VInterpTo(Velocity, FVector::ZeroVector, DeltaSeconds, 8.f);
			return;
		}
	}

	// ===== 스프링-댐퍼(자연스러운 쫄래쫄래) =====
	// Accel = (Desired - Current)*K - Velocity*C
	FVector Error = (Desired - Current);

	FVector Accel = Error * SpringStrength - Velocity * Damping;

	// 가속도/속도 클램프(튀는 것 방지)
	Accel = Accel.GetClampedToMaxSize(MaxAccel);

	Velocity += Accel * DeltaSeconds;
	Velocity = Velocity.GetClampedToMaxSize(MaxSpeed);

	const FVector NewLoc = Current + Velocity * DeltaSeconds;
	SetActorLocation(NewLoc);
}

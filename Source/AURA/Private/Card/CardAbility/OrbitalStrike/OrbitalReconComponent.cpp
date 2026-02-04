// Fill out your copyright notice in the Description page of Project Settings.


#include "Card/CardAbility/OrbitalStrike/OrbitalReconComponent.h"
#include "Card/CardAbility/OrbitalStrike/OrbitalReconActor.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"


UOrbitalReconComponent::UOrbitalReconComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


void UOrbitalReconComponent::BeginPlay()
{
	Super::BeginPlay();

	// 기본으로는 C++ Actor 사용. 나중에 BP_ReconActor로 바꾸고 싶으면 여기서 지정하거나 에디터에서 설정.
	if (!ReconViewActorClass)
	{
		ReconViewActorClass = AOrbitalReconActor::StaticClass();
	}
	
}


void UOrbitalReconComponent::ExecuteReconScan(const FVector& Center, float Radius, bool bCombatMode)
{
	UWorld* World = GetWorld();
	if (!World) return;

	const float UseRadius = (Radius > 0.f) ? Radius : DefaultScanRadius;

	// 스냅샷 기본 정보 저장(전투/비전투 모두 동일)
	LastSnapshot.Center = Center;
	LastSnapshot.Radius = UseRadius;
	LastSnapshot.RevealedActors.Reset();

	// 대상 수집
	TArray<AActor*> Targets;
	CollectTargetsInSphere(Center, UseRadius, Targets);

	// 전투중: “표시/노출/미니맵 핑” 처리 (실제 구현은 다음 단계에서 붙이면 됨)
	// 비전투: HUD에서 볼 스냅샷만 저장해도 충분
	for (AActor* A : Targets)
	{
		if (!PassesFilter(A)) continue;

		LastSnapshot.RevealedActors.Add(A);

		// TODO(다음 단계):
		// - 클로킹 유닛 노출: CustomDepth/Material param/Tag 부여
		// - 약점 하이라이트: Weakpoint 컴포넌트/태그 기반 표시
		// - 미니맵 핑: HUD 데이터로 전달
		// 여기서는 “수집/캐싱”만 해 둔다.
	}

	// bCombatMode=false면 “OpenReconView”로 상공 화면 진입 가능
}

bool UOrbitalReconComponent::OpenReconView()
{
	UWorld* World = GetWorld();
	if (!World) return false;

	APlayerController* PC = GetOwningPC();
	if (!PC) return false;

	// 이미 열려있으면 true
	if (SpawnedViewActor)
	{
		PC->SetViewTargetWithBlend(SpawnedViewActor, 0.2f);
		return true;
	}

	if (!ReconViewActorClass) return false;

	FActorSpawnParameters Params;
	Params.Owner = GetOwner();
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	SpawnedViewActor = World->SpawnActor<AOrbitalReconActor>(ReconViewActorClass, Params);
	if (!SpawnedViewActor) return false;

	SpawnedViewActor->InitView(LastSnapshot.Center, LastSnapshot.Radius);

	// 카메라 전환
	PC->SetViewTargetWithBlend(SpawnedViewActor, 0.2f);
	PC->bShowMouseCursor = true;

	// TODO(다음 단계):
	// - 입력 라우팅(확대/이동/닫기 키)
	// - 전투 진입 시 강제 종료

	return true;
}

void UOrbitalReconComponent::CloseReconView()
{
	APlayerController* PC = GetOwningPC();
	if (PC)
	{
		// 기본은 PC가 소유한 Pawn으로 복귀
		if (APawn* Pawn = PC->GetPawn())
		{
			PC->SetViewTargetWithBlend(Pawn, 0.2f);
		}
		PC->bShowMouseCursor = false;
	}

	if (SpawnedViewActor)
	{
		SpawnedViewActor->Destroy();
		SpawnedViewActor = nullptr;
	}
}

void UOrbitalReconComponent::CollectTargetsInSphere(const FVector& Center, float Radius,
	TArray<AActor*>& OutActors) const
{
	OutActors.Reset();

	// “수집”만 단순하게: 월드 액터들을 순회 → 거리 체크
	// (나중에 SphereOverlapActors로 최적화 가능)
	UWorld* World = GetWorld();
	if (!World) return;

	const float R2 = Radius * Radius;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* A = *It;
		if (!A) continue;

		if (FVector::DistSquared(A->GetActorLocation(), Center) <= R2)
		{
			OutActors.Add(A);
		}
	}
}

bool UOrbitalReconComponent::PassesFilter(AActor* Actor) const
{
	if (!Actor) return false;
	if (Actor == GetOwner()) return false;

	// 인스티게이터/플레이어 제외 옵션
	if (bExcludeInstigatorPawn)
	{
		if (const APawn* P = Cast<APawn>(Actor))
		{
			if (P == CachedInstigatorPawn.Get()) return false;
		}
	}

	if (bExcludePlayerControlledPawns)
	{
		if (const APawn* P = Cast<APawn>(Actor))
		{
			if (P->IsPlayerControlled()) return false;
		}
	}

	// 배경맵 제외(고정 지형): Root Mobility != Movable 제외
	if (bExcludeNonMovableRoot)
	{
		USceneComponent* Root = Actor->GetRootComponent();
		if (!Root || Root->Mobility != EComponentMobility::Movable)
		{
			return false;
		}
	}

	// 태그 기반 그룹 필터: Enemy/Summoned/Prop 중 하나라도 해당되면 통과
	const bool bGroup =
		(!EnemyActorTag.IsNone() && Actor->ActorHasTag(EnemyActorTag)) ||
		(!SummonedActorTag.IsNone() && Actor->ActorHasTag(SummonedActorTag)) ||
		(!PropActorTag.IsNone() && Actor->ActorHasTag(PropActorTag));

	return bGroup;
}

APlayerController* UOrbitalReconComponent::GetOwningPC() const
{
	// PlayerState에 붙는 전제: Owner의 Pawn/Controller 접근은 프로젝트마다 다를 수 있으니
	// 가장 안전한 방식: 첫 번째 로컬 PC를 가져옴 (싱글/로컬플레이 기준)
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	return UGameplayStatics::GetPlayerController(World, 0);
}

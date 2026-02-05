// Fill out your copyright notice in the Description page of Project Settings.


#include "Card/CardAbility/OrbitalStrike/OrbitalReconComponent.h"
#include "Card/CardAbility/OrbitalStrike/OrbitalReconActor.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

#include "Blueprint/UserWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Character/PlayerCharacter.h"
#include "Engine/LocalPlayer.h"


UOrbitalReconComponent::UOrbitalReconComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
	// Recon IMC
	if (!ReconIMC)
	{
		static ConstructorHelpers::FObjectFinder<UInputMappingContext> IMC_Finder(
			TEXT("/Game/_BP/Input/Recon/IMC_Recon.IMC_Recon")
		);
		if (IMC_Finder.Succeeded())
		{
			ReconIMC = IMC_Finder.Object;
		}
	}

	// Recon Overlay Widget
	if (!ReconOverlayWidgetClass)
	{
		
		static ConstructorHelpers::FClassFinder<UUserWidget> WBP_Finder(
			TEXT("/Game/_BP/UI/Recon/WBP_ReconOverlay")
		);
		if (WBP_Finder.Succeeded())
		{
			ReconOverlayWidgetClass = WBP_Finder.Class;
		}
	}
}


void UOrbitalReconComponent::BeginPlay()
{
	Super::BeginPlay();

	// 기본으로는 C++ Actor 사용. 나중에 BP_ReconActor로 바꾸고 싶으면 여기서 지정하거나 에디터에서 설정.
	if (!ReconViewActorClass)
	{
		ReconViewActorClass = AOrbitalReconActor::StaticClass();
	}
	
	// 기본 인스티게이터 캐싱(싱글/로컬 기준)
	if (APlayerController* PC = GetOwningPC())
	{
		CachedInstigatorPawn = PC->GetPawn();
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
	UE_LOG(LogTemp, Warning, TEXT("[ReconComp] OpenReconView ENTER Owner=%s PC=%s ReconViewActorClass=%s OverlayClass=%s IMC=%s"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(GetOwningPC()),
		*GetNameSafe(ReconViewActorClass),
		*GetNameSafe(ReconOverlayWidgetClass),
		*GetNameSafe(ReconIMC));
	
	UWorld* World = GetWorld();
	if (!World) return false;

	APlayerController* PC = GetOwningPC();
	if (!PC) return false;
	
	// ===== Spawn Transform (플레이어 머리 위) =====
	APawn* AvatarPawn = PC->GetPawn();
	const FVector BaseLoc = AvatarPawn ? AvatarPawn->GetActorLocation() : FVector::ZeroVector;

	// 캐릭터 “딱 머리 위”: X/Y 동일, Z만 올림
	FVector SpawnLoc = BaseLoc;
	SpawnLoc.Z += 1000.f;   // 원하는 값(가까우면 800~1000 사이로 튜닝)

	// 회전은 yaw만 맞추되, 위치는 정수평으로 고정
	const float Yaw = AvatarPawn ? AvatarPawn->GetActorRotation().Yaw : 0.f;
	const FRotator SpawnRot(0.f, Yaw, 0.f);
	
	

	// 복구용 저장(최초 1회만)
	if (!PrevViewTarget.IsValid())
	{
		PrevViewTarget = PC->GetViewTarget();
		bPrevShowMouseCursor = PC->bShowMouseCursor;
	}

	// ===== Character Mesh 처리 =====
	if (APawn* Pawn = PC->GetPawn())
	{
		if (APlayerCharacter* PCChar = Cast<APlayerCharacter>(Pawn))
		{
			// --- 스냅샷 저장 ---
			if (USkeletalMeshComponent* Body = PCChar->GetMesh())
			{
				CharacterMeshSnapshot.bBodyOwnerNoSee   = Body->bOwnerNoSee;
				CharacterMeshSnapshot.bBodyVisible      = Body->IsVisible();
				CharacterMeshSnapshot.bBodyHiddenInGame = Body->bHiddenInGame;
			}

			if (USkeletalMeshComponent* Arms = PCChar->FindComponentByClass<USkeletalMeshComponent>())
			{
				CharacterMeshSnapshot.bArmsVisible      = Arms->IsVisible();
				CharacterMeshSnapshot.bArmsHiddenInGame = Arms->bHiddenInGame;
			}

			// --- Recon 전용 상태 적용 ---
			PCChar->EnterReconView();
		}
	}
	
	// ViewActor 준비(없으면 스폰, 있으면 재사용)
	if (!SpawnedViewActor)
	{
		if (!ReconViewActorClass)
		{
			return false;
		}

		FActorSpawnParameters Params;
		Params.Owner = GetOwner();
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		SpawnedViewActor = World->SpawnActor<AOrbitalReconActor>(ReconViewActorClass, SpawnLoc, SpawnRot, Params);
		if (!SpawnedViewActor) return false;
		
		UE_LOG(LogTemp, Warning, TEXT("[ReconComp] SpawnedViewActor=%s"), *GetNameSafe(SpawnedViewActor));
	}
	
	// 이미 존재해도 진입 시점마다 플레이어 기준으로 위치 갱신
	SpawnedViewActor->SetActorLocation(SpawnLoc);
	SpawnedViewActor->SetActorRotation(SpawnRot);
	SpawnedViewActor->SetMoveReferenceYaw(Yaw);
	
	
	// 최신 스냅샷 기준으로 뷰 초기화
	SpawnedViewActor->InitView(LastSnapshot.Center, LastSnapshot.Radius);

	// 카메라 전환
	PC->SetViewTargetWithBlend(SpawnedViewActor, 0.2f);

	// ===== IMC_Recon 추가 =====
	ULocalPlayer* LP = PC->GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* Subsys = LP ? LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;

	UE_LOG(LogTemp, Warning, TEXT("[ReconComp] LP=%s Subsys=%s ReconIMC=%s"),
		*GetNameSafe(LP),
		*GetNameSafe(Subsys),
		*GetNameSafe(ReconIMC));

	if (ReconIMC && Subsys)
	{
		Subsys->AddMappingContext(ReconIMC, 100);

		// ★ 중요: 즉시 매핑 리빌드 (간헐적으로 “추가됐는데 입력 안 먹음” 방지)
		Subsys->RequestRebuildControlMappings(FModifyContextOptions(), EInputMappingRebuildType::Rebuild);
	}

	// UI 표시(깡통)
	if (!ReconOverlayWidget && ReconOverlayWidgetClass)
	{
		ReconOverlayWidget = CreateWidget<UUserWidget>(PC, ReconOverlayWidgetClass);
		if (ReconOverlayWidget)
		{
			ReconOverlayWidget->AddToViewport(1000);
		}
	}

	// 커서/입력 모드
	PC->bShowMouseCursor = true;

	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(false);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

	if (ReconOverlayWidget)
	{
		Mode.SetWidgetToFocus(ReconOverlayWidget->TakeWidget());
	}

	PC->SetInputMode(Mode);

	return true;
}

void UOrbitalReconComponent::CloseReconView()
{
	APlayerController* PC = GetOwningPC();
	if (!PC) return;

	// IMC 제거
	if (ReconIMC)
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				Subsys->RemoveMappingContext(ReconIMC);
			}
		}
	}

	// UI 제거
	if (ReconOverlayWidget)
	{
		ReconOverlayWidget->RemoveFromParent();
		ReconOverlayWidget = nullptr;
	}

	// ViewTarget 복원(저장된 게 있으면 우선)
	if (PrevViewTarget.IsValid())
	{
		PC->SetViewTargetWithBlend(PrevViewTarget.Get(), 0.2f);
		PrevViewTarget.Reset();
	}
	else
	{
		// 예외: 저장이 없으면 Pawn 복귀
		if (APawn* Pawn = PC->GetPawn())
		{
			PC->SetViewTargetWithBlend(Pawn, 0.2f);
		}
	}

	// 커서/입력 모드 복원
	PC->bShowMouseCursor = bPrevShowMouseCursor;

	FInputModeGameOnly Mode;
	PC->SetInputMode(Mode);
	
	// ===== Character Mesh 복원 =====
	if (APawn* Pawn = PC->GetPawn())
	{
		if (APlayerCharacter* PCChar = Cast<APlayerCharacter>(Pawn))
		{
			PCChar->ExitReconView();
		}
	}
}

AOrbitalReconActor* UOrbitalReconComponent::GetReconViewActor() const
{
	return SpawnedViewActor;
}

bool UOrbitalReconComponent::IsReconViewOpen() const
{
	// ReconOverlayWidget가 떠있거나, ViewActor가 있으면 열린 것으로 간주
	return (ReconOverlayWidget != nullptr) || (SpawnedViewActor != nullptr);
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



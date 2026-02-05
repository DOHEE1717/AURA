// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OrbitalReconComponent.generated.h"

class AOrbitalReconActor;
class APawn;
class APlayerController;
class UInputMappingContext;
class UUserWidget;

USTRUCT(BlueprintType)
struct FOrbitalReconSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FVector Center = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	float Radius = 0.f;

	UPROPERTY(BlueprintReadOnly)
	TArray<TObjectPtr<AActor>> RevealedActors;
};

USTRUCT()
struct FReconCharacterMesh
{
	GENERATED_BODY()

	bool bBodyOwnerNoSee = true;
	bool bBodyVisible = false;
	bool bBodyHiddenInGame = true;

	bool bArmsVisible = true;
	bool bArmsHiddenInGame = false;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AURA_API UOrbitalReconComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	
	UOrbitalReconComponent();

	// ===== 스캔 실행 (Alt Ability에서 호출할 함수) =====
	// bCombatMode=true: 전투용(약점/클로킹/미니맵 핑)만
	// bCombatMode=false: 비전투용(스냅샷 저장 + HUD/상공뷰 진입 가능)
	UFUNCTION(BlueprintCallable, Category="Recon")
	void ExecuteReconScan(const FVector& Center, float Radius, bool bCombatMode);

	// ===== 비전투 전용: 정찰 화면 열기/닫기 =====
	UFUNCTION(BlueprintCallable, Category="Recon")
	bool OpenReconView();

	UFUNCTION(BlueprintCallable, Category="Recon")
	void CloseReconView();

	// 최근 스캔 결과(HUD에서 읽기)
	UFUNCTION(BlueprintCallable, Category="Recon")
	const FOrbitalReconSnapshot& GetLastSnapshot() const { return LastSnapshot; }
	

	AOrbitalReconActor* GetReconViewActor() const;
	bool IsReconViewOpen() const;

protected:
	virtual void BeginPlay() override;

protected:
	// 비전투 상공 화면용 Actor 클래스(BP로 대체 가능)
	UPROPERTY(EditDefaultsOnly, Category="Recon|View")
	TSubclassOf<AOrbitalReconActor> ReconViewActorClass;

	// 스캔 반경 기본값(기획상 넓게)
	UPROPERTY(EditDefaultsOnly, Category="Recon|Tuning")
	float DefaultScanRadius = 1200.f;

	// ===== 대상 필터(태그 기반) =====
	UPROPERTY(EditDefaultsOnly, Category="Recon|Filter")
	FName EnemyActorTag = "Unit.Enemy";

	UPROPERTY(EditDefaultsOnly, Category="Recon|Filter")
	FName SummonedActorTag = "Unit.Summoned";

	UPROPERTY(EditDefaultsOnly, Category="Recon|Filter")
	FName PropActorTag = "Object.Prop";

	// 배경맵 제외 규칙 유지: Root Mobility != Movable 제외
	UPROPERTY(EditDefaultsOnly, Category="Recon|Filter")
	bool bExcludeNonMovableRoot = true;

	// 인스티게이터/플레이어 제외 옵션(필요하면 켜기)
	UPROPERTY(EditDefaultsOnly, Category="Recon|Filter")
	bool bExcludeInstigatorPawn = true;

	UPROPERTY(EditDefaultsOnly, Category="Recon|Filter")
	bool bExcludePlayerControlledPawns = false;
	
	// ===== 비전투 ReconView 입력/UI =====

	// 비전투 ReconView에서만 활성화할 IMC
	UPROPERTY(EditDefaultsOnly, Category="Recon|Input")
	TObjectPtr<UInputMappingContext> ReconIMC = nullptr;

	// 비전투 ReconView 오버레이(깡통 위젯)
	UPROPERTY(EditDefaultsOnly, Category="Recon|UI")
	TSubclassOf<UUserWidget> ReconOverlayWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ReconOverlayWidget = nullptr;

	// ViewTarget/커서 복구용
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> PrevViewTarget;

	UPROPERTY(Transient)
	bool bPrevShowMouseCursor = false;

private:
	UPROPERTY()
	TObjectPtr<AOrbitalReconActor> SpawnedViewActor = nullptr;

	FOrbitalReconSnapshot LastSnapshot;

	TWeakObjectPtr<APawn> CachedInstigatorPawn;

	// 내부 구현
	void CollectTargetsInSphere(const FVector& Center, float Radius, TArray<AActor*>& OutActors) const;
	bool PassesFilter(AActor* Actor) const;

	APlayerController* GetOwningPC() const;
	
private:
	FReconCharacterMesh CharacterMeshSnapshot;
		
};

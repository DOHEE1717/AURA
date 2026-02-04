// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OrbitalStrikeActor.generated.h"

class USphereComponent;
class UDecalComponent;
class UMaterialInterface;
class APawn;

UCLASS()
class AURA_API AOrbitalStrikeActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AOrbitalStrikeActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

protected:
	// ===== 범위(Overlap) =====
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="OrbitalStrike")
	TObjectPtr<USphereComponent> Sphere;

	// ===== 시각화(지면 데칼: “조사 범위” 표시) =====
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="OrbitalStrike|Visual")
	TObjectPtr<UDecalComponent> AreaDecal;

	// 에디터에서 MI_OrbitalStrikeDecal 등 지정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OrbitalStrike|Visual")
	TObjectPtr<UMaterialInterface> AreaDecalMaterial;

	// 데칼 크기(UE 규칙: X=두께, Y=가로, Z=세로)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OrbitalStrike|Visual")
	FVector DecalSize = FVector(600.f, 600.f, 600.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OrbitalStrike|Visual")
	float DecalZOffset = 5.f;

protected:
	// ===== 튜닝 =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="OrbitalStrike|Tuning")
	float Radius = 200.f;

	// 지속시간 (레이저 조사 시간)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OrbitalStrike|Tuning")
	float LifeTime = 3.0f;

	// 피해 적용 주기(초) (0이면 매 Tick)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OrbitalStrike|Tuning")
	float DamageInterval = 0.2f;

	// 1회 Tick당 피해량
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OrbitalStrike|Tuning")
	float DamagePerTick = 10.f;

	// ApplyDamage에 전달될 데미지 타입(필요 시 지정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OrbitalStrike|Tuning")
	TSubclassOf<UDamageType> DamageTypeClass;

protected:
	// ===== 필터 =====
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OrbitalStrike|Filter")
	bool bExcludeInstigatorPawn = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OrbitalStrike|Filter")
	bool bExcludePlayerControlledPawns = true;

	// 특정 태그는 피해 제외 (예: "NoOrbitalDamage")
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OrbitalStrike|Filter")
	FName IgnoreTag = "NoOrbitalDamage";

	// ===== Target Group Switch (GravityFieldActor와 동일한 컨셉) =====
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OrbitalStrike|Target")
	bool bAffectEnemies = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OrbitalStrike|Target")
	bool bAffectSummoned = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OrbitalStrike|Target")
	bool bAffectProps = true;

	// 그룹 판정용 Actor Tags (액터의 Tags 배열에 넣는 값)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OrbitalStrike|Target")
	FName EnemyActorTag = "Unit.Enemy";

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OrbitalStrike|Target")
	FName SummonedActorTag = "Unit.Summoned";

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OrbitalStrike|Target")
	FName PropActorTag = "Object.Prop";

private:
	UPROPERTY()
	TObjectPtr<APawn> CachedInstigatorPawn = nullptr;

	float TimeAccum = 0.f;

	// 틱 1회당 피해 적용
	void ApplyDamageOnce();

	bool ShouldIgnoreActor(const AActor* Actor) const;
	bool PassesGroupFilter(const AActor* Actor) const;

};

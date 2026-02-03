// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GravityFieldReverseActor.generated.h"

class USphereComponent;
class UPrimitiveComponent;
class ACharacter;
class UDecalComponent;
class UMaterialInterface;

UCLASS()
class AURA_API AGravityFieldReverseActor : public AActor
{
	GENERATED_BODY()
	
public:
	AGravityFieldReverseActor();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

protected:
	// Overlap 범위(역중력장 반경)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GravityField")
	TObjectPtr<USphereComponent> Sphere;

	// ★ 지속 데칼
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GravityField|Visual")
	TObjectPtr<UDecalComponent> FieldDecal;

	// 에디터에서 MI_GravityDecal_Blue(또는 Alt 전용) 지정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Visual")
	TObjectPtr<UMaterialInterface> FieldDecalMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Visual")
	FVector DecalSize = FVector(600.f, 600.f, 600.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Visual")
	float DecalZOffset = 5.f;

	// ===== 튜닝 파라미터 =====
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Tuning")
	float Radius = 900.f;
	
	// ===== Hover(부유) =====
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Hover")
	bool bEnableHoverCeiling = true;
	
	// ===== Hover(부유) - Physics =====
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Hover")
	bool bEnableHoverForPhysics = true;

	// 물리 오브젝트 HoverZ 근처에서 작동하는 밴드(±). 캐릭터와 별도로 둠
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Hover")
	float PhysicsHoverBand = 120.f;

	// (물리) 스프링 강도: dz(목표-현재)로 가속도를 만듦
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Hover")
	float PhysicsHoverSpringK = 8.f;

	// (물리) 감쇠: 현재 VelZ를 줄여서 흔들림/튀는 것 방지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Hover")
	float PhysicsHoverDamping = 6.f;

	// (물리) 최대 가속도(= AddForce bAccelChange 사용 시 값이 가속도)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Hover")
	float MaxPhysicsHoverAccel = 2500.f;

	// 목표 부유 높이(월드 Z).  예: 필드 생성 위치 + 350
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Hover")
	float HoverZOffsetFromField = 350.f;

	// HoverZ 근처에서만 “정지” 제어를 강하게 거는 밴드(±)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Hover")
	float HoverBand = 80.f;

	// (캐릭터) 목표 높이로 수렴시키는 스프링 강도(클수록 빨리 멈춤)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Hover")
	float HoverSpringK = 12.f;

	// (캐릭터) 수직 속도 감쇠(클수록 흔들림 줄고 바로 멈춤)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Hover")
	float HoverDamping = 8.f;

	// (캐릭터) 수직 보정 속도 상한(너무 튀는 것 방지)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Hover")
	float MaxHoverUpSpeed = 900.f;

	// 물리 오브젝트(사물) “위로” 힘
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Tuning")
	float PhysicsUpForce = 180000.f;

	// 캐릭터(적) “위로” 띄우는 강도 (LaunchCharacter)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Tuning")
	float CharacterLiftStrength = 2400.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Tuning")
	float LifeTime = 2.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Tuning")
	float ApplyInterval = 0.02f;

	// ===== Filter =====
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Filter")
	bool bExcludeInstigatorPawn = false;          // 내 캐릭터도 적용하려면 false

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Filter")
	bool bExcludePlayerControlledPawns = false;   // PlayerControlled도 적용하려면 false

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Filter")
	FName IgnoreTag = "NoGravityPull";            // 이 태그만 예외(원하면 None으로)

	// ===== Target Group Switch =====
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Target")
	bool bAffectEnemies = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Target")
	bool bAffectSummoned = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Target")
	bool bAffectProps = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Target")
	FName EnemyActorTag = "Unit.Enemy";

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Target")
	FName SummonedActorTag = "Unit.Summoned";

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Target")
	FName PropActorTag = "Object.Prop";

	

private:
	UPROPERTY()
	TObjectPtr<APawn> CachedInstigatorPawn = nullptr;

	float TimeAccum = 0.f;
	float CachedHoverZ = 0.f;
	float LastTickDelta = 0.f;

	void ApplyLiftOnce();
	bool ShouldIgnoreActor(const AActor* Actor) const;

};

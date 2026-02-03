// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GravityFieldActor.generated.h"

class USphereComponent;
class UPrimitiveComponent;
class ACharacter;
class UDecalComponent;
class UMaterialInterface;

UCLASS()
class AURA_API AGravityFieldActor : public AActor
{
	GENERATED_BODY()
	
public:
	AGravityFieldActor();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

protected:
	// Overlap 범위(중력장 반경)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GravityField")
	TObjectPtr<USphereComponent> Sphere;
	
	//중력장 시각화 
	// ★ 지속 데칼
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GravityField|Visual")
	TObjectPtr<UDecalComponent> FieldDecal;

	// 에디터에서 MI_GravityDecal_Blue 지정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Visual")
	TObjectPtr<UMaterialInterface> FieldDecalMaterial;

	// 데칼 크기(가로/세로/두께)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Visual")
	FVector DecalSize = FVector(600.f, 600.f, 600.f); // X=두께, Y=가로, Z=세로(UE 데칼 규칙)

	// 바닥에서 살짝 띄우기(Z-Fighting 방지)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Visual")
	float DecalZOffset = 5.f;

	// ===== 튜닝 파라미터 =====
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Tuning")
	float Radius = 900.f;

	// 물리 오브젝트(사물) 끌어당김 강도 (Force)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Tuning")
	float PhysicsForce = 180000.f;

	// 캐릭터(적) 끌어당김 강도 (LaunchCharacter 가속 느낌)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Tuning")
	float CharacterPullStrength = 2400.f;

	// 중력장 지속시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Tuning")
	float LifeTime = 2.5f;

	// Tick에서 당김 적용 빈도(성능용). 0이면 매 Tick.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Tuning")
	float ApplyInterval = 0.02f;

	// 플레이어 제외 여부(기본 true)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Filter")
	bool bExcludeInstigatorPawn = true;

	// 특정 태그가 있으면 당기지 않음(예: "NoGravityPull")
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Filter")
	FName IgnoreTag = "NoGravityPull";
	
	// ===== Target Group Switch =====
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Target")
	bool bAffectEnemies = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Target")
	bool bAffectSummoned = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Target")
	bool bAffectProps = true;

	// 그룹 판정용 "Actor Tag" 이름(에디터에서 액터 Tags에 넣을 값)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Target")
	FName EnemyActorTag = "Unit.Enemy";

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Target")
	FName SummonedActorTag = "Unit.Summoned";

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Target")
	FName PropActorTag = "Object.Prop";

	// 플레이어 컨트롤 Pawn은 제외(Instigator가 아닐 수도 있으니 강제)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GravityField|Filter")
	bool bExcludePlayerControlledPawns = true;

private:
	// 캐시
	UPROPERTY()
	TObjectPtr<APawn> CachedInstigatorPawn = nullptr;

	float TimeAccum = 0.f;

	// 대상 수집/필터
	void ApplyPullOnce();

	bool ShouldIgnoreActor(const AActor* Actor) const;

};

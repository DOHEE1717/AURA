#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HealingDroneActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class AURA_API AHealingDroneActor : public AActor
{
	GENERATED_BODY()

public:
	AHealingDroneActor();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Drone")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Drone")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	// 따라갈 대상 (Ability에서 SpawnParams.Owner로 들어온 Owner를 기본으로 사용)
	UPROPERTY()
	TWeakObjectPtr<AActor> FollowTarget;

	// ===== Follow Tuning =====

	// 기본적으로 플레이어 기준 오프셋 (로컬 forward/right/up)
	UPROPERTY(EditDefaultsOnly, Category="Drone|Follow")
	FVector LocalOffset = FVector(60.f, 40.f, 120.f);

	// 이 거리 이상 벌어지면 “추적 시작”
	UPROPERTY(EditDefaultsOnly, Category="Drone|Follow")
	float FollowStartDistance = 40.f;

	// 이 거리 안으로 들어오면 “추적 멈추고 유지”
	UPROPERTY(EditDefaultsOnly, Category="Drone|Follow")
	float FollowStopDistance = 20.f;

	// 스프링 강도(클수록 빠르게 붙음)
	UPROPERTY(EditDefaultsOnly, Category="Drone|Follow")
	float SpringStrength = 14.f;

	// 감쇠(클수록 덜 출렁이고 빨리 멈춤)
	UPROPERTY(EditDefaultsOnly, Category="Drone|Follow")
	float Damping = 8.f;

	// 최대 이동 속도
	UPROPERTY(EditDefaultsOnly, Category="Drone|Follow")
	float MaxSpeed = 900.f;

	// 최대 가속도(너무 크면 튐)
	UPROPERTY(EditDefaultsOnly, Category="Drone|Follow")
	float MaxAccel = 5000.f;

	// ===== Hover (부유) =====
	UPROPERTY(EditDefaultsOnly, Category="Drone|Hover")
	float HoverAmplitude = 8.f;

	UPROPERTY(EditDefaultsOnly, Category="Drone|Hover")
	float HoverFrequency = 1.5f;

private:
	// 현재 속도(스프링-댐퍼 적분)
	FVector Velocity = FVector::ZeroVector;

	// “추적 중” 상태
	bool bIsFollowing = false;

	float HoverTime = 0.f;

private:
	// 실제 따라갈 목표 위치 계산 (플레이어 위치 + 회전 기준 오프셋 + 호버)
	FVector ComputeDesiredLocation(float DeltaSeconds);

	// 스프링-댐퍼 방식으로 위치 업데이트
	void UpdateFollow(float DeltaSeconds);

	// 타겟 자동획득(Owner/Instigator/PlayerPawn fallback)
	void ResolveFollowTarget();
};
